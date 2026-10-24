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
static unsigned long errorShownAt = 0;
constexpr unsigned long ERROR_DISPLAY_MS = 2000;

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
      errorShownAt = 0;
      alarmStop();
      reminderStop();
      lockDoor();
      ledSet(false);
      showLocked();
      break;

    case STATE_UNLOCKED:
      alarmStop();
      unlockDoor();
      ledSet(false);
      showUnlocked();
      beepOnce(100);
      doorOpenSince = 0;
      break;

    case STATE_PIN_INPUT:
      showPinInput(pinLength());
      break;

    case STATE_SECURITY_LOCKOUT:
      pinClear();
      alarmStart();
      showLockout(LOCKOUT_DURATION_MS / 1000);
      break;

    case STATE_TAMPER_ALARM:
      alarmStart();
      ledBlinkStart();
      showTamper();
      break;

    case STATE_DOOR_AJAR_WARNING:
      reminderStart();
      showDoorAjar();
      break;
  }
}

// ---------- Xử lý xác thực thất bại ----------
void onAuthFail(const char* msg) {
  if (failCount >= FAIL_COUNT_LOCKOUT) {
    enterState(STATE_SECURITY_LOCKOUT);
    return;
  }
  failCount++;
  if (state != STATE_LOCKED) enterState(STATE_LOCKED);
  showError(msg); // ghi đè màn hình "LOCKED"
  beepError();
  errorShownAt = millis();
}

// ---------- Xử lý theo từng trạng thái (chạy mỗi vòng loop) ----------
void handleLocked() {
  if (doorIsOpen()) { 
    enterState(STATE_TAMPER_ALARM);
    return;
  }
  
  if (errorShownAt != 0 && millis() - errorShownAt > ERROR_DISPLAY_MS) {
    errorShownAt = 0;
    showLocked();
  }

  char key;
  InputEvent evt = pollInput(key);

  switch (evt) {
    case EVT_CARD_VALID:
      failCount = 0;
      enterState(STATE_UNLOCKED);
      break;
    case EVT_CARD_INVALID:
      onAuthFail("The khong dung");
      break;
    case EVT_KEY_DIGIT:
      pinClear();
      pinAppend(key);
      enterState(STATE_PIN_INPUT);
      break;
    default:
      break;
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

void handlePinInput() {
  char key;
  InputEvent evt = pollInput(key);

  switch (evt) {
    case EVT_KEY_DIGIT:
      if (pinAppend(key)) showPinInput(pinLength());
      break;

    case EVT_KEY_STAR: // '*' = xóa PIN đang nhập
      pinClear();
      showPinInput(0);
      break;

    case EVT_KEY_HASH: // '#' = xác nhận
      if (pinLength() == 0) break;
      if (pinMatchesHome()) {
        failCount = 0;
        pinClear();
        enterState(STATE_UNLOCKED);
      } else {
        pinClear();
        onAuthFail("Sai ma PIN");
      }
      return;

    default: // quẹt thẻ lúc đang nhập PIN: bỏ qua
      break;
  }

  // Quá 10s không bấm phím -> hủy PIN
  if (millis() - pinLastKeyTime() > PIN_TIMEOUT_MS) {
    pinClear();
    enterState(STATE_LOCKED);
  }
}

void handleSecurityLockout() {
  unsigned long elapsed = millis() - stateEnterTime;

  if (elapsed >= LOCKOUT_DURATION_MS) {
    alarmStop();
    failCount = 0;
    enterState(STATE_LOCKED);
    return;
  }

  // Cập nhật đồng hồ đếm ngược mỗi giây
  static uint8_t lastShown = 255;
  uint8_t left = (LOCKOUT_DURATION_MS - elapsed + 999) / 1000;
  if (left != lastShown) {
    lastShown = left;
    showLockout(left);
  }
}

void handleTamperAlarm() {
  char key;
  InputEvent evt = pollInput(key);

  switch (evt) {
    case EVT_CARD_VALID: // quẹt đúng thẻ -> tắt báo động
      failCount = 0;
      pinClear();
      enterState(STATE_UNLOCKED);
      return;

    case EVT_KEY_DIGIT:
      pinAppend(key);
      break;

    case EVT_KEY_STAR:
      pinClear();
      break;

    case EVT_KEY_HASH:
      if (pinLength() == 0) break;
      if (pinMatchesHome()) {
        failCount = 0;
        pinClear();
        enterState(STATE_UNLOCKED);
        return;
      }
      pinClear(); // sai PIN: báo động vẫn tiếp tục
      break;

    default: // thẻ sai: báo động vẫn tiếp tục
      break;
  }

  if (pinLength() > 0 && millis() - pinLastKeyTime() > PIN_TIMEOUT_MS) {
    pinClear();
  }
}

void handleDoorAjar() {
  if (!doorIsOpen()) {
    enterState(STATE_LOCKED);
  }
}

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
