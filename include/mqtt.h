#pragma once
#include <Arduino.h>

// ===== Lớp kết nối =====
void mqttInit(); // WiFi + broker
void mqttLoop();
bool mqttConnected();
bool mqttPublish(const char* topic, const char* payload, bool retained = false);

// ===== Lớp bản tin JSON =====
void publishState(); // topic ct295/smart-access-control/state
void publishAccess(const char* method, const char* uid, bool success); // topic access
void publishAlert(const char* type, const char* severity,
                  const char* message, const char* action); // topic alert