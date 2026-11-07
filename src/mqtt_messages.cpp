#include "mqtt.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "alerts_door.h"

namespace
{

  void publishJson(const char *topic, const JsonDocument &document)
  {
    if (document.overflowed())
    {
      Serial.println("[MQTT] JSON allocation failed");
      return;
    }
    String payload;
    const size_t size = measureJson(document);
    if (!payload.reserve(size + 1) || serializeJson(document, payload) != size)
    {
      Serial.println("[MQTT] JSON serialization failed");
      return;
    }
    mqttPublish(topic, payload.c_str());
  }

  void startTimeSync()
  {
    static bool started = false;
    if (!started && WiFi.status() == WL_CONNECTED)
    {
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      started = true;
    }
  }

  String utcTimestamp()
  {
    startTimeSync();
    const time_t now = time(nullptr);
    if (now < 1704067200)
      return "";
    struct tm utc;
    char timestamp[21];
    if (!gmtime_r(&now, &utc) ||
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0)
    {
      return "";
    }
    return String(timestamp);
  }
}

void publishState()
{
  startTimeSync();
  // Chi LOCKED/UNLOCKED doi goc servo trong FSM hien tai.
  // Cac trang thai khac giu nguyen lenh chot truoc do.
  static bool lockIsLocked = true;
  if (state == STATE_LOCKED)
    lockIsLocked = true;
  if (state == STATE_UNLOCKED)
    lockIsLocked = false;

  JsonDocument document;
  document["device_id"] = DEVICE_ID;
  document["state"] = stateName(state);
  document["door_physical"] = doorIsOpen() ? "OPEN" : "CLOSED";
  document["lock_status"] = lockIsLocked ? "LOCKED" : "UNLOCKED";
  document["failed_attempts"] = failCount;
  document["wifi_rssi"] = WiFi.RSSI();
  document["uptime_sec"] = millis() / 1000UL;
  publishJson(TOPIC_STATE, document);
}

void publishAccess(const char *method, const char *uid, bool success)
{
  JsonDocument document;
  document["timestamp"] = utcTimestamp();
  document["method"] = method ? method : ""; // Vi du: RFID_CARD, PIN
  document["card_uid"] = uid ? uid : "";     // Khong truyen ma PIN vao day.
  document["result"] = success ? "SUCCESS" : "FAILURE";
  publishJson(TOPIC_ACCESS, document);
}

void publishAlert(const char *type, const char *severity,
                  const char *message, const char *action)
{
  JsonDocument document;
  document["timestamp"] = utcTimestamp();
  document["alert_type"] = type ? type : "";
  document["severity"] = severity ? severity : "";
  document["message"] = message ? message : "";
  document["action_taken"] = action ? action : "";
  publishJson(TOPIC_ALERT, document);
}
