/**
 * @file Remote_ESP8266_Bridge.cpp
 * @brief ESP8266 bridge between Mega UART frames and ESP-NOW transport.
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <GenericLogger.h>
#include <GenericOTA.h>
#include <Std_Types.h>

const char *ssid = "BOSS T4 NGOAI 2G";
const char *password = "d12345678";

typedef struct struct_message
{
  uint8_t lx;
  uint8_t ly;
  uint8_t rx;
  uint8_t ry;
  uint8_t buttons;
} struct_message;

struct_message myData;

// Broadcast để kiểm thử khi chưa cấu hình MAC riêng của robot.
uint8_t robotMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

ASSIGN_LOG_MACROS(RemoteESP8266Bridge, Serial);

// Callback khi nhận dữ liệu từ ESP32-C3
void OnDataRecv(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len)
{
  RemoteESP8266Bridge_LogD("Data received from MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  if (len == sizeof(myData))
  {
    memcpy(&myData, incomingData, sizeof(myData));

    // Log dữ liệu nhận được
    RemoteESP8266Bridge_LogD("Received -> lx:%d, ly:%d, rx:%d, ry:%d, btn:%d",
                             myData.lx, myData.ly, myData.rx, myData.ry, myData.buttons);
  }
}

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000;
unsigned long counter = 0;

uint8_t CalculateChecksum(const uint8_t *buffer, size_t size)
{
  uint8_t checksum = 0;
  for (size_t index = 0; index < size; index++)
  {
    checksum ^= buffer[index];
  }
  return checksum;
}

void SendControlPacket(const ControlPacket &packet)
{
  const uint8_t *payload = reinterpret_cast<const uint8_t *>(&packet);
  uint8_t checksum = CalculateChecksum(payload, sizeof(ControlPacket));

  Serial.write(0xAA);
  Serial.write(0xFF);
  Serial.write(payload, sizeof(ControlPacket));
  Serial.write(checksum);
  Serial.write(0x55);
}

void ProcessControlPacket(const ControlPacket &packet)
{
  RemoteESP8266Bridge_LogD("RX type=%u throttle=%d strafe=%d rotation=%d buttons=0x%02X seq=%u",
                           packet.msgType,
                           packet.throttle,
                           packet.strafe,
                           packet.rotation,
                           packet.buttons,
                           packet.sequenceNum);

  SendControlPacket(packet);

  uint8_t result = esp_now_send(robotMac,
                                reinterpret_cast<uint8_t *>(const_cast<ControlPacket *>(&packet)),
                                sizeof(ControlPacket));
  if (result != 0)
  {
    RemoteESP8266Bridge_LogE("Send to robot failed: %u", result);
  }
}

void ProcessMegaSerial()
{
  constexpr size_t packetSize = sizeof(ControlPacket);
  constexpr size_t frameSize = 2 + packetSize + 1 + 1;
  static uint8_t frame[frameSize];
  static size_t frameIndex = 0;
  static char command[16];
  static size_t commandLength = 0;

  while (Serial.available())
  {
    uint8_t byte = static_cast<uint8_t>(Serial.read());

    if (frameIndex == 0 && byte != 0xAA)
    {
      if (byte == '\n' || byte == '\r')
      {
        command[commandLength] = '\0';
        if (strcmp(command, "ESP_RESET") == 0)
        {
          RemoteESP8266Bridge_LogW("Reset command received");
          Serial.flush();
          delay(20);
          ESP.restart();
        }
        commandLength = 0;
      }
      else if (byte >= 32 && byte <= 126 && commandLength < sizeof(command) - 1)
      {
        command[commandLength++] = static_cast<char>(byte);
      }
      continue;
    }

    if (frameIndex == 0)
    {
      if (byte == 0xAA)
      {
        frame[frameIndex++] = byte;
      }
      continue;
    }

    if (frameIndex == 1)
    {
      if (byte == 0xFF)
      {
        frame[frameIndex++] = byte;
      }
      else
      {
        frameIndex = 0;
      }
      continue;
    }

    frame[frameIndex++] = byte;
    if (frameIndex == frameSize)
    {
      frameIndex = 0;

      if (frame[frameSize - 1] == 0x55 &&
          CalculateChecksum(&frame[2], packetSize) == frame[2 + packetSize])
      {
        ControlPacket packet;
        memcpy(&packet, &frame[2], packetSize);
        if (IsControlPacketValid(packet) &&
            (packet.msgType == CONTROL_MESSAGE || packet.msgType == HEARTBEAT_MESSAGE))
        {
          ProcessControlPacket(packet);
        }
        else
        {
          RemoteESP8266Bridge_LogW("Discarded invalid control packet");
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  RemoteESP8266Bridge_LogI("ESP8266: DUAL MODE (WIFI + OTA + ESP-NOW)");

  // Khởi tạo ESP-NOW trước; OTA/WiFi chạy nền và không được chặn remote.
  if (esp_now_init() != 0)
  {
    RemoteESP8266Bridge_LogF("ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  if (esp_now_add_peer(robotMac, ESP_NOW_ROLE_COMBO, 0, nullptr, 0) != 0)
  {
    RemoteESP8266Bridge_LogE("ESP-NOW robot peer setup failed");
  }
  esp_now_register_recv_cb(OnDataRecv);
  RemoteESP8266Bridge_LogI("ESP-NOW initialized and ready");

  GenericOTA::begin(ssid, password, "esp8266-mega-wifi", WIFI_AP_STA);
  RemoteESP8266Bridge_LogI("Current WiFi channel: %d", WiFi.channel());
  RemoteESP8266Bridge_LogI("My MAC: %s", WiFi.macAddress().c_str());

  lastSendTime = millis();
}

void loop()
{
  GenericOTA::handle();
  ProcessMegaSerial();
}