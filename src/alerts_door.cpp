#include "config.h"
#include "alerts_door.h"

static unsigned long beepEndTime = 0;

void alertsInit() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP);
}

void alertsUpdate() {
  if (beepEndTime != 0 && (long)(millis() - beepEndTime) >= 0) {
    digitalWrite(BUZZER_PIN, LOW);
    beepEndTime = 0;
  }
  // TODO(C): cập nhật pattern hú báo động, bíp nhắc nhở, LED chớp
}

void beepOnce(unsigned long ms) {
  digitalWrite(BUZZER_PIN, HIGH);
  beepEndTime = millis() + ms;
}

// TODO(C): đang là bản tạm
void beepError()     { beepOnce(600); }
void alarmStart()    { /* TODO(C): hú còi liên tục & pattern báo động */ }
void alarmStop()     {}
void reminderStart() { /* TODO(C): bíp ngắt quãng */ }
void reminderStop()  {}

void ledSet(bool on)  { digitalWrite(LED_PIN, on ? HIGH : LOW); }
void ledBlinkStart()  { /* TODO(C): chớp LED bằng millis() */ }
void ledBlinkStop()   {}

bool doorIsOpen() {
  // TODO(C): thêm chống dội (debounce ~50ms). Hiện đọc thẳng.
  return digitalRead(DOOR_PIN) == HIGH;
}
