// ====================================================================
//  ----- Khung FSM -----
//  Quy tắc:
//   1. Mọi chuyển trạng thái chỉ qua enterState().
//   2. Không dùng delay().
//   3. Việc phần cứng nằm trong module riêng, main.cpp chỉ điều phối.
// ====================================================================
#include <Arduino.h>
#include "config.h"
#include "auth_input.h"
#include "outputs.h"
#include "alerts_door.h"

State state = STATE_LOCKED;
unsigned long stateEnterTime = 0;
int failCount = 0;
static unsigned long doorOpenSince = 0;

const char* stateName(State s) {
  switch (s) {
    case STATE_LOCKED:            return "STATE_LOCKED";
    case STATE_PIN_INPUT:         return "STATE_PIN_INPUT";
    case STATE_UNLOCKED:          return "STATE_UNLOCKED";
    case STATE_SECURITY_LOCKOUT:  return "STATE_SECURITY_LOCKOUT";
    case STATE_TAMPER_ALARM:      return "STATE_TAMPER_ALARM";
    case STATE_DOOR_AJAR_WARNING: return "STATE_DOOR_AJAR_WARNING";
  }
  return "UNKNOWN";
}

// ---------- Entry action: chạy 1 lần khi vào trạng thái ----------
void enterState(State next) {
  Serial.printf("[FSM] %s -> %s\r\n", stateName(state), stateName(next));
  state = next;
  stateEnterTime = millis();

  switch (state) {
    case STATE_LOCKED:
      lockDoor();
      ledSet(false);
      showLocked();
      break;

    case STATE_UNLOCKED:
      unlockDoor();
      ledSet(true); // TODO(): LED đỏ dùng cho báo động, cần đổi sau
      showUnlocked();
      beepOnce(100);
      doorOpenSince = 0;
      break;

    case STATE_PIN_INPUT:         // TODO(A): pinClear(), showPinInput(0)
      break;
    case STATE_SECURITY_LOCKOUT:  // TODO(A+B+C): alarmStart(), khóa 30s, showLockout()
      break;
    case STATE_TAMPER_ALARM:      // TODO(C+B): alarmStart(), ledBlinkStart(), showTamper()
      break;
    case STATE_DOOR_AJAR_WARNING: // TODO(C+B): reminderStart(), showDoorAjar()
      break;
  }
}

// ---------- Xử lý theo từng trạng thái (chạy mỗi vòng loop) ----------
void handleLocked() {
  char key;
  InputEvent evt = pollInput(key);

  if (evt == EVT_CARD_VALID) {
    failCount = 0;
    enterState(STATE_UNLOCKED);
  }
  // TODO(A): EVT_CARD_INVALID -> failCount++, showError, beepError
  // TODO(A): EVT_KEY_DIGIT -> pinClear(), pinAppend(key), enterState(STATE_PIN_INPUT)
  // TODO(C): doorIsOpen() khi không có lệnh mở -> enterState(STATE_TAMPER_ALARM)
  else if (evt == EVT_KEY_HASH) {
    // Để test tạm thời: '#' mở cửa. Xóa khi làm xong PIN_INPUT.
    enterState(STATE_UNLOCKED);
  }
}

void handleUnlocked() {
  if (doorIsOpen()) {
    if (doorOpenSince == 0) doorOpenSince = millis();
    if (millis() - doorOpenSince > AJAR_TIMEOUT_MS) enterState(STATE_DOOR_AJAR_WARNING);
    return;
  }
  doorOpenSince = 0;
  if (millis() - stateEnterTime > UNLOCK_DURATION_MS) enterState(STATE_LOCKED);
}

void handlePinInput()        { /* TODO(A): nhận digit, '#' kiểm tra PIN, timeout PIN_TIMEOUT_MS */ }
void handleSecurityLockout() { /* TODO(A): hết LOCKOUT_DURATION_MS -> failCount=0, về LOCKED */ }
void handleTamperAlarm()     { /* TODO(C): xử lý/thoát báo động cạy cửa */ }
void handleDoorAjar()        { /* TODO(C): doorIsOpen()==false -> reminderStop(), về LOCKED */ }

// ---------- Arduino ----------
void setup() {
  Serial.begin(115200);
  alertsInit();
  authInit();
  outputsInit();
  enterState(STATE_LOCKED); // trạng thái khởi đầu
}

void loop() {
  alertsUpdate();

  switch (state) {
    case STATE_LOCKED:            handleLocked();          break;
    case STATE_PIN_INPUT:         handlePinInput();        break;
    case STATE_UNLOCKED:          handleUnlocked();        break;
    case STATE_SECURITY_LOCKOUT:  handleSecurityLockout(); break;
    case STATE_TAMPER_ALARM:      handleTamperAlarm();     break;
    case STATE_DOOR_AJAR_WARNING: handleDoorAjar();        break;
  }
}
