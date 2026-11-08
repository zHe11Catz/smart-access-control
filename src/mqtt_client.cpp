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
    snprintf(clientId, sizeof(clientId), "%s-%04X%08X-%04X",
             MQTT_CLIENT_ID,
             (uint16_t)(mac >> 32),
             (uint32_t)mac,
             (uint16_t)random(0x1000, 0xFFFF));
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
  char testPayload[256];
  snprintf(testPayload, sizeof(testPayload),
           "{\"device_id\":\"%s\",\"client_id\":\"%s\",\"status\":\"connected\",\"message\":\"Ket noi HiveMQ thanh cong\",\"uptime_sec\":%lu}",
           DEVICE_ID, getClientId(), millis() / 1000);
  mqttPublish(TOPIC_BASE "/test", testPayload, true);
}

void mqttInit() {
  randomSeed(micros());
  Serial.println("[WIFI] Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Connecting to SSID '%s'...\r\n", WIFI_SSID);

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
  mqttClient.setKeepAlive(60); // 60s keepalive giup ket noi on dinh, khong bi timeout som

  lastReconnectAttempt = 0;
}

void mqttLoop() {
  unsigned long now = millis();

  // Hỗ trợ phím tắt test trên Wokwi Terminal: 'x' ngắt MQTT, 'w' ngắt WiFi
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'x' || c == 'X') {
      Serial.println(F("[TEST] Da ngat ket noi MQTT thu cong! Xem tu ket noi lai sau 5s..."));
      mqttClient.disconnect();
    } else if (c == 'w' || c == 'W') {
      Serial.println(F("[TEST] Da ngat ket noi WiFi thu cong! Xem tu ket noi lai sau 5s..."));
      WiFi.disconnect();
    }
  }

  // 1. Kiểm tra trạng thái WiFi (non-blocking)
  static bool wifiWasConnected = false;
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false;
      Serial.println("[WIFI] Connection lost! Waiting for WiFi...");
    }
    // Tự thử kết nối lại WiFi mỗi 5s
    if (now - lastReconnectAttempt >= MQTT_RETRY_MS) {
      lastReconnectAttempt = now;
      Serial.println("[WIFI] Reconnecting to WiFi...");
      WiFi.reconnect();
    }
    return;
  }

  if (!wifiWasConnected) {
    wifiWasConnected = true;
    Serial.printf("[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
  }

  // 2. Kiểm tra kết nối MQTT Broker
  static bool mqttWasConnected = false;
  if (!mqttClient.connected()) {
    if (mqttWasConnected) {
      mqttWasConnected = false;
      Serial.printf("[MQTT] Connection lost (rc=%d)! Will retry in %lu ms...\r\n",
                    mqttClient.state(), MQTT_RETRY_MS);
    }

    // Tự kết nối lại mỗi 5s (MQTT_RETRY_MS)
    if (lastReconnectAttempt == 0 || now - lastReconnectAttempt >= MQTT_RETRY_MS) {
      lastReconnectAttempt = now;
      Serial.printf("[MQTT] Connecting to broker %s:%u as '%s'...\r\n",
                    MQTT_BROKER, MQTT_PORT, getClientId());

      const char* willTopic = TOPIC_STATUS;
      const char* willPayload = "{\"device_id\":\"" DEVICE_ID "\",\"status\":\"offline\"}";
      if (mqttClient.connect(getClientId(), willTopic, 0, true, willPayload)) {
        mqttWasConnected = true;
        Serial.println("[MQTT] Connected to HiveMQ successfully!");

        // Publish status online theo đặc tả LWT
        char statusPayload[128];
        snprintf(statusPayload, sizeof(statusPayload),
                 "{\"device_id\":\"%s\",\"status\":\"online\"}",
                 DEVICE_ID);
        mqttPublish(TOPIC_STATUS, statusPayload, true);

        // Gửi bản tin thử nghiệm để MQTT Explorer nhận được ngay
        publishTestMessage();
      } else {
        Serial.printf("[MQTT] Connection failed, rc=%d. Retrying in %lu ms...\r\n",
                      mqttClient.state(), MQTT_RETRY_MS);
      }
    }
  } else {
    mqttWasConnected = true;
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