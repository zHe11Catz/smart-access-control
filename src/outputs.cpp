#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include "config.h"
#include "outputs.h"

static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static Servo lockServo;

void outputsInit() {
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  lockServo.attach(SERVO_PIN);
}

// ---------- Servo ----------
void lockDoor()   { lockServo.write(SERVO_LOCK_ANGLE); }
void unlockDoor() { lockServo.write(SERVO_UNLOCK_ANGLE); }

// ---------- OLED ----------
void showOled(const String& l1, const String& l2) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);  oled.println(l1);
  oled.setCursor(0, 20); oled.println(l2);
  oled.display();
}

void showLocked()   { showOled("Smart Access", "Locked"); }
void showUnlocked() { showOled("Smart Access", "Unlocked"); }

// TODO(B): các hàm dưới đang là bản đơn giản, làm đẹp nếu cần thiết
void showPinInput(uint8_t len) {
  String stars = "";
  for (uint8_t i = 0; i < len; i++) stars += '*';
  showOled("Nhap ma PIN:", stars);
}

void showLockout(uint8_t secondsLeft) {
  showOled("LOCKED", String(secondsLeft) + "s");
}

void showError(const char* msg) { showOled("Loi:", msg); }
void showTamper()               { showOled("CANH BAO!", "Cay cua"); }
void showDoorAjar()             { showOled("Chu y", "Quen dong cua"); }
