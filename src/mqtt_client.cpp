#include "mqtt.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

static unsigned long lastReconnectAttempt = 0;
static char clientId[64] = {0};

static const char* getClientId() {
  if (clientId[0] == '\0') {
    uint64_t mac = ESP.getEfuseMac();
    snprintf(clientId, sizeof(clientId), "%s-%04X%08X",
             MQTT_CLIENT_ID,
             (uint16_t)(mac >> 32),
             (uint32_t)mac);
  }
  return clientId;
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] Message arrived on [%s]: ", topic);
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

static void publishTestMessage() {
  char testPayload[192];
  snprintf(testPayload, sizeof(testPayload),
           "{\"device\":\"%s\",\"clientId\":\"%s\",\"status\":\"connected\",\"message\":\"Ket noi HiveMQ thanh cong\",\"uptime\":%lu}",
           DEVICE_ID, getClientId(), millis() / 1000);
  mqttPublish(TOPIC_BASE "/test", testPayload, true);
}

void mqttInit() {
  Serial.println("[WIFI] Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Connecting to SSID '%s'...\r\n", WIFI_SSID);

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  lastReconnectAttempt = 0;
}

void mqttLoop() {
  unsigned long now = millis();

  // 1. Kiểm tra trạng thái WiFi (non-blocking)
  static bool wifiWasConnected = false;
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false;
      Serial.println("[WIFI] Connection lost! Waiting for WiFi...");
    }
    return;
  }

  if (!wifiWasConnected) {
    wifiWasConnected = true;
    Serial.printf("[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
  }

  // 2. Kiểm tra kết nối MQTT Broker
  if (!mqttClient.connected()) {
    // Tự kết nối lại mỗi 5s (MQTT_RETRY_MS)
    if (lastReconnectAttempt == 0 || now - lastReconnectAttempt >= MQTT_RETRY_MS) {
      lastReconnectAttempt = now;
      Serial.printf("[MQTT] Connecting to broker %s:%u as '%s'...\r\n",
                    MQTT_BROKER, MQTT_PORT, getClientId());

      if (mqttClient.connect(getClientId())) {
        Serial.println("[MQTT] Connected to HiveMQ successfully!");
        // Gửi bản tin thử nghiệm để MQTT Explorer nhận được ngay
        publishTestMessage();
      } else {
        Serial.printf("[MQTT] Connection failed, rc=%d. Retrying in %lu ms...\r\n",
                      mqttClient.state(), MQTT_RETRY_MS);
      }
    }
  } else {
    // Đã kết nối, duy trì vòng lặp xử lý gói tin
    mqttClient.loop();


  }
}

bool mqttConnected() {
  return mqttClient.connected();
}

bool mqttPublish(const char* topic, const char* payload, bool retained) {
  if (!mqttClient.connected()) {
    Serial.printf("[MQTT] Publish skipped (not connected): %s\r\n", topic);
    return false;
  }
  bool success = mqttClient.publish(topic, payload, retained);
  if (success) {
    Serial.printf("[MQTT] Published [%s]: %s\r\n", topic, payload);
  } else {
    Serial.printf("[MQTT] Publish failed [%s]\r\n", topic);
  }
  return success;
}