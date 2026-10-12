#pragma once
// ============================================================
//  config.h - Cấu hình dùng chung (chân, thời gian, enum State)
//  CẢ NHÓM dùng chung file này. Sửa xong phải báo nhóm.
// ============================================================
#include <Arduino.h>

// ---------- Chân GPIO  ----------
#define SS_PIN     5
#define RST_PIN    27
#define SERVO_PIN  13
#define BUZZER_PIN 15
#define LED_PIN    2
#define DOOR_PIN   4 // INPUT_PULLUP: LOW = cửa đóng, HIGH = cửa hở

// ---------- Thời gian (ms) theo bảng FSM ----------
constexpr unsigned long UNLOCK_DURATION_MS  = 5000;   // mở cửa 5s rồi tự khóa
constexpr unsigned long PIN_TIMEOUT_MS      = 10000;  // quá 10s không bấm -> hủy PIN
constexpr unsigned long LOCKOUT_DURATION_MS = 30000;  // khóa an ninh 30s
constexpr unsigned long AJAR_TIMEOUT_MS     = 30000;  // cửa mở quá 30s -> nhắc nhở

// ---------- Servo ----------
constexpr int SERVO_LOCK_ANGLE   = 0;
constexpr int SERVO_UNLOCK_ANGLE = 90;

// ---------- Bảo mật ----------
// Sai mã khi failCount >= 2 -> SECURITY_LOCKOUT (lần sai thứ 3)
constexpr int FAIL_COUNT_LOCKOUT = 2;
const char HOME_PIN[] = "1234"; // PIN chủ nhà

// ---------- Máy trạng thái ----------
enum State {
  STATE_LOCKED,
  STATE_PIN_INPUT,
  STATE_UNLOCKED,
  STATE_SECURITY_LOCKOUT,
  STATE_TAMPER_ALARM,
  STATE_DOOR_AJAR_WARNING
};

// Biến toàn cục định nghĩa trong main.cpp
extern int failCount;

// Pattern cho Còi/LED 
constexpr unsigned long ALARM_BEEP_MS      = 200;  // nhịp hú
constexpr unsigned long REMINDER_BEEP_MS   = 100;  // độ dài mỗi bíp nhắc
constexpr unsigned long REMINDER_PERIOD_MS = 2000; // chu kỳ bíp nhắc
constexpr unsigned long LED_BLINK_MS       = 250;
constexpr unsigned long DOOR_DEBOUNCE_MS   = 50;