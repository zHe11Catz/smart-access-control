#include "mqtt.h"

void mqttInit() {}
void mqttLoop() {}
bool mqttConnected() { return false; }
bool mqttPublish(const char* topic, const char* payload, bool retained) {
  Serial.printf("[MQTT-STUB] %s -> %s\r\n", topic, payload);
  return false;
}