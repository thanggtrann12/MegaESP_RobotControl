#include <ESP8266WiFi.h>
#include <espnow.h>
#include <GenericOTA.h>
#include <Std_Types.h>

const char* ssid = "BOSS T4 NGOAI 2G";
const char* password = "d12345678";

void ProcessMegaCommand() {
  static char command[16];
  static size_t commandLength = 0;

  while (Serial.available()) {
    char character = static_cast<char>(Serial.read());
    if (character == '\n' || character == '\r') {
      command[commandLength] = '\0';
      if (strcmp(command, "ESP_RESET") == 0) {
        Serial.println(F("[ROBOT ESP] Reset command received"));
        Serial.flush();
        delay(20);
        ESP.restart();
      }
      commandLength = 0;
    } else if (character >= 32 && character <= 126 &&
               commandLength < sizeof(command) - 1) {
      command[commandLength++] = character;
    }
  }
}

void OnControlPacketReceived(uint8_t* macAddress, uint8_t* incomingData, uint8_t length) {
  if (length != sizeof(ControlPacket)) {
    Serial.printf("[ROBOT] Invalid ESP-NOW length: %u\n", length);
    return;
  }

  ControlPacket packet;
  memcpy(&packet, incomingData, sizeof(packet));

  Serial.write(0xAA);
  Serial.write(0xFF);
  Serial.write(incomingData, sizeof(ControlPacket));

  uint8_t checksum = 0;
  for (size_t index = 0; index < sizeof(ControlPacket); index++) {
    checksum ^= incomingData[index];
  }
  Serial.write(checksum);
  Serial.write(0x55);
}


void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n==============================================");
  Serial.println("  ESP8266: DUAL MODE  ");
  Serial.println("==============================================");
  GenericOTA::begin(ssid, password, "robot-esp8266", WIFI_AP_STA);

  if (esp_now_init() != 0) {
    Serial.println("[ROBOT] ESP-NOW init failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnControlPacketReceived);
  Serial.println("[ROBOT] ESP-NOW receiver ready");
}

void loop() {
  GenericOTA::handle();
  ProcessMegaCommand();
}