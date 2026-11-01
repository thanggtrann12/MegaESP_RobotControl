/**
 * @file Remote_ESP8266_Bridge.cpp
 * @brief ESP8266 bridge between Mega UART frames and ESP-NOW transport.
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <GenericLogger.h>
#include <GenericOTA.h>
#include <ControlPacket.h>
#include <ComManager.h>

//======================================= Constants and Global Objects ========================//
const char *ssid = "BOSS T4 NGOAI 2G";
const char *password = "d12345678";

// Broadcast để kiểm thử khi chưa cấu hình MAC riêng của robot.
static uint8_t robotMac[] = {0x84, 0xC7, 0xBB, 0x5A, 0x70, 0x88};
static ComManager comBridge(Serial);

// Log goes to the UART (Mega) and to a TCP client on port 23, so it can be read over WiFi/OTA.
// The last LOG_RING_SIZE bytes are replayed when a client connects, so boot logs are not lost.
class NetLogStream : public Stream
{
public:
  void begin()
  {
    _server.begin();
    _server.setNoDelay(true);
  }

  void handle()
  {
    if (_server.hasClient())
    {
      if (_client && _client.connected())
      {
        _client.stop();
      }
      _client = _server.available();
      for (size_t i = 0; i < _len; ++i)
      {
        _client.write(_ring[(_head + RING_SIZE - _len + i) % RING_SIZE]);
      }
    }
  }

  size_t write(uint8_t byte) override { return write(&byte, 1); }

  size_t write(const uint8_t *buffer, size_t size) override
  {
    Serial.write(buffer, size);
    for (size_t i = 0; i < size; ++i)
    {
      _ring[_head] = buffer[i];
      _head = (_head + 1) % RING_SIZE;
      if (_len < RING_SIZE)
      {
        ++_len;
      }
    }
    if (_client && _client.connected())
    {
      _client.write(buffer, size);
    }
    return size;
  }

  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }

private:
  static constexpr size_t RING_SIZE = 2048;
  WiFiServer _server{23};
  WiFiClient _client;
  uint8_t _ring[RING_SIZE];
  size_t _head = 0;
  size_t _len = 0;
};

static NetLogStream netLog;

//======================================== Logging Macros ====================================//
INIT_LOG(netLog);
ASSIGN_LOG_MACROS(RemoteESP8266Bridge);

//======================================== Function Prototypes ===============================//
void subModuleInit();
void subModuleUpdate();
void onRobotDataReceived(uint8_t *macAddress, uint8_t *incomingData, uint8_t length);
void forwardPacket(const ControlPacket &packet);
void onRobotDataSent(uint8_t *macAddress, uint8_t status);
void flushPendingPacket();
void updatePacket();
void updateCommand();

//======================================== Setup and Loop Functions ==========================//
void setup()
{
  comBridge.Init(115200);
  delay(100);
  subModuleInit();
}

void loop()
{
  subModuleUpdate();
}

void subModuleInit()
{
  LOG_I("Initializing sub-modules...");

  // Khởi tạo ESP-NOW trước; OTA/WiFi chạy nền và không được chặn remote.
  if (esp_now_init() != 0)
  {
    LOG_F("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  if (esp_now_add_peer(robotMac, ESP_NOW_ROLE_COMBO, 0, nullptr, 0) != 0)
  {
    LOG_E("ESP-NOW robot peer setup failed");
  }
  esp_now_register_recv_cb(onRobotDataReceived);
  esp_now_register_send_cb(onRobotDataSent);
  LOG_I("ESP-NOW initialized");

  GenericOTA::begin(ssid, password, "esp8266-mega-wifi", WIFI_AP_STA);
  netLog.begin();
  LOG_I("My MAC: %s", WiFi.macAddress().c_str());
  LOG_I("Sub-modules initialized successfully");
}

void subModuleUpdate()
{
  GenericOTA::handle();
  netLog.handle();
  static uint8_t reportedChannel = 0;
  const uint8_t channel = WiFi.status() == WL_CONNECTED ? WiFi.channel() : 0;
  if (channel != reportedChannel)
  {
    reportedChannel = channel;
    if (channel != 0)
    {
      LOG_I("Connected WiFi channel: %u", channel);
    }
  }
  updatePacket();
  flushPendingPacket();
  updateCommand();
}

//======================================== ESP-NOW & Communication Functions =================//
void onRobotDataReceived(uint8_t *macAddress, uint8_t *incomingData, uint8_t length)
{
  LOG_D("Data received from MAC: %02x:%02x:%02x:%02x:%02x:%02x (%u bytes)",
        macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5], length);
}

// ESP8266 allows one ESP-NOW send in flight: the next one is rejected until the send callback fires.
static volatile bool sendBusy = false;
static uint32_t sendStartMs = 0;
static bool hasPending = false;
static ControlPacket pendingPacket;
static uint32_t sentCount = 0;
static uint32_t replacedCount = 0;
static uint32_t failCount = 0;
static uint32_t fromMegaCount = 0;
static uint32_t heartbeatCount = 0;
static uint32_t ackFailCount = 0;
static int lastError = 0;
static uint32_t lastReportMs = 0;

void onRobotDataSent(uint8_t *macAddress, uint8_t status)
{
  (void)macAddress;
  if (status != 0)
  {
    ackFailCount++; // robot did not acknowledge the frame (wrong channel / robot off)
  }
  sendBusy = false;
}

static void sendToRobot(const ControlPacket &packet)
{
  const int result = esp_now_send(robotMac,
                                  reinterpret_cast<uint8_t *>(const_cast<ControlPacket *>(&packet)),
                                  sizeof(ControlPacket));
  if (result == 0)
  {
    sendBusy = true;
    sendStartMs = millis();
    sentCount++;
  }
  else
  {
    failCount++;
    lastError = result;
  }
}

static void reportSendStats()
{
  const uint32_t now = millis();
  if (now - lastReportMs < 1000)
  {
    return;
  }
  lastReportMs = now;
  LOG_I("1s: fromMega=%lu (heartbeat=%lu) sent=%lu ackFail=%lu replaced=%lu sendErr=%lu (last %d)",
        static_cast<unsigned long>(fromMegaCount), static_cast<unsigned long>(heartbeatCount),
        static_cast<unsigned long>(sentCount), static_cast<unsigned long>(ackFailCount),
        static_cast<unsigned long>(replacedCount), static_cast<unsigned long>(failCount), lastError);
  sentCount = 0;
  replacedCount = 0;
  failCount = 0;
  fromMegaCount = 0;
  heartbeatCount = 0;
  ackFailCount = 0;
}

// Gửi packet về Mega (echo) và sang robot qua ESP-NOW
void forwardPacket(const ControlPacket &packet)
{
  comBridge.SendPacket(packet);
  fromMegaCount++;
  if (packet.msgType == HEARTBEAT_MESSAGE)
  {
    heartbeatCount++;
  }

  if (sendBusy)
  {
    // Newest packet wins: stale control values are useless to the robot.
    if (hasPending)
    {
      replacedCount++;
    }
    pendingPacket = packet;
    hasPending = true;
    return;
  }
  sendToRobot(packet);
}

void flushPendingPacket()
{
  // Recover if a send callback was lost (e.g. during a WiFi channel change).
  if (sendBusy && millis() - sendStartMs > 20)
  {
    sendBusy = false;
  }
  if (!sendBusy && hasPending)
  {
    hasPending = false;
    sendToRobot(pendingPacket);
  }
  reportSendStats();
}

void updatePacket()
{
  ControlPacket packet;
  while (comBridge.ReadPacket(packet))
  {
    if (packet.msgType == CONTROL_MESSAGE || packet.msgType == HEARTBEAT_MESSAGE)
    {
      forwardPacket(packet);
    }
    else
    {
      LOG_W("Discarded invalid control packet");
    }
  }
}

void updateCommand()
{
  char command[16];
  if (comBridge.ReadLog(command, sizeof(command)) && strcmp(command, "ESP_RESET") == 0)
  {
    LOG_W("Reset command received");
    Serial.flush();
    delay(20);
    ESP.restart();
  }
}