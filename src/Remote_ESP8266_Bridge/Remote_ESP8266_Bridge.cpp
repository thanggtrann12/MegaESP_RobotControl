#include <ESP8266WiFi.h>
#include <espnow.h>
#include <GenericOTA.h>
#include <Std_Types.h>

const char* ssid = "BOSS T4 NGOAI 2G";
const char* password = "d12345678";

typedef struct struct_message {
  uint8_t lx;
  uint8_t ly;
  uint8_t rx;
  uint8_t ry;
  uint8_t buttons;
} struct_message;

struct_message myData;

// Broadcast để kiểm thử khi chưa cấu hình MAC riêng của robot.
uint8_t robotMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Callback khi nhận dữ liệu từ ESP32-C3
void OnDataRecv(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len) {
  Serial.printf("[ESP-NOW] Data received from MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));

    // Log dữ liệu nhận được
    Serial.printf("[ESP-NOW] Received -> lx:%d, ly:%d, rx:%d, ry:%d, btn:%d\n", 
                  myData.lx, myData.ly, myData.rx, myData.ry, myData.buttons);
  }
}

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000;
unsigned long counter = 0;

uint8_t CalculateChecksum(const uint8_t* buffer, size_t size) {
  uint8_t checksum = 0;
  for (size_t index = 0; index < size; index++) {
    checksum ^= buffer[index];
  }
  return checksum;
}

void SendControlPacket(const ControlPacket& packet) {
  const uint8_t* payload = reinterpret_cast<const uint8_t*>(&packet);
  uint8_t checksum = CalculateChecksum(payload, sizeof(ControlPacket));

  Serial.write(0xAA);
  Serial.write(0xFF);
  Serial.write(payload, sizeof(ControlPacket));
  Serial.write(checksum);
  Serial.write(0x55);
}

void ProcessControlPacket(const ControlPacket& packet) {
  Serial.printf("[BRIDGE] RX J1(%d, %d) J2(%d, %d) buttons=0x%04X\n",
                packet.joy1_x,
                packet.joy1_y,
                packet.joy2_x,
                packet.joy2_y,
                packet.buttons);

  SendControlPacket(packet);

  uint8_t result = esp_now_send(robotMac,
                                reinterpret_cast<uint8_t*>(const_cast<ControlPacket*>(&packet)),
                                sizeof(ControlPacket));
  if (result != 0) {
    Serial.printf("[ESP-NOW] Send to robot failed: %u\n", result);
  }
}

void ProcessMegaSerial() {
  constexpr size_t packetSize = sizeof(ControlPacket);
  constexpr size_t frameSize = 2 + packetSize + 1 + 1;
  static uint8_t frame[frameSize];
  static size_t frameIndex = 0;
  static char command[16];
  static size_t commandLength = 0;

  while (Serial.available()) {
    uint8_t byte = static_cast<uint8_t>(Serial.read());

    if (frameIndex == 0 && byte != 0xAA) {
      if (byte == '\n' || byte == '\r') {
        command[commandLength] = '\0';
        if (strcmp(command, "ESP_RESET") == 0) {
          Serial.println(F("[BRIDGE] Reset command received"));
          Serial.flush();
          delay(20);
          ESP.restart();
        }
        commandLength = 0;
      } else if (byte >= 32 && byte <= 126 && commandLength < sizeof(command) - 1) {
        command[commandLength++] = static_cast<char>(byte);
      }
      continue;
    }

    if (frameIndex == 0) {
      if (byte == 0xAA) {
        frame[frameIndex++] = byte;
      }
      continue;
    }

    if (frameIndex == 1) {
      if (byte == 0xFF) {
        frame[frameIndex++] = byte;
      } else {
        frameIndex = 0;
      }
      continue;
    }

    frame[frameIndex++] = byte;
    if (frameIndex == frameSize) {
      frameIndex = 0;

      if (frame[frameSize - 1] == 0x55 &&
          CalculateChecksum(&frame[2], packetSize) == frame[2 + packetSize]) {
        ControlPacket packet;
        memcpy(&packet, &frame[2], packetSize);
        ProcessControlPacket(packet);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n==============================================");
  Serial.println("  ESP8266: DUAL MODE (WIFI + OTA + ESP-NOW)   ");
  Serial.println("==============================================");

  // Khởi tạo ESP-NOW trước; OTA/WiFi chạy nền và không được chặn remote.
  if (esp_now_init() != 0) {
    Serial.println("[ERROR] ESPNow Init Failed!");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  if (esp_now_add_peer(robotMac, ESP_NOW_ROLE_COMBO, 0, nullptr, 0) != 0) {
    Serial.println("[ERROR] ESP-NOW robot peer setup failed");
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("[SYSTEM] ESP-NOW initialized & Ready!");

  GenericOTA::begin(ssid, password, "esp8266-mega-wifi", WIFI_AP_STA);
  Serial.printf("Current WiFi Channel: %d (MUST MATCH ESP32-C3!)\n", WiFi.channel());
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());

  lastSendTime = millis();
}

void loop() {
  GenericOTA::handle();
  ProcessMegaSerial();
}