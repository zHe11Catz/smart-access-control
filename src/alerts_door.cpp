#include "config.h"
#include "alerts_door.h"

// Trạng thái còi (Buzzer)
static unsigned long beepEndTime = 0;

static bool alarmActive = false;
static bool alarmBuzzerState = false;
static unsigned long lastAlarmToggle = 0;

static bool reminderActive = false;
static bool reminderBuzzerState = false;
static unsigned long reminderCycleStart = 0;

// Trạng thái LED
static bool ledBlinkActive = false;
static bool ledBlinkState = false;
static unsigned long lastLedToggle = 0;

// Trạng thái công tắc cửa
static bool doorDebouncedState = false; // true = OPEN (HIGH), false = CLOSED (LOW)
static int lastRawDoorReading = HIGH;
static unsigned long lastDoorDebounceTime = 0;

void alertsInit()
{
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  int raw = digitalRead(DOOR_PIN);
  lastRawDoorReading = raw;
  doorDebouncedState = (raw == HIGH);
  lastDoorDebounceTime = millis();
}

void alertsUpdate()
{
  // 1. Chống dội công tắc cửa (Debounce ~50ms)
  int rawDoor = digitalRead(DOOR_PIN);
  if (rawDoor != lastRawDoorReading)
  {
    lastRawDoorReading = rawDoor;
    lastDoorDebounceTime = millis();
  }
  if ((millis() - lastDoorDebounceTime) >= DOOR_DEBOUNCE_MS)
  {
    doorDebouncedState = (rawDoor == HIGH);
  }

  // 2. Tính mức còi mong muốn theo ưu tiên: Alarm > Reminder > BeepOnce
  bool buzzerOn = false;

  if (alarmActive)
  {
    if (millis() - lastAlarmToggle >= ALARM_BEEP_MS)
    {
      lastAlarmToggle = millis();
      alarmBuzzerState = !alarmBuzzerState;
    }
    buzzerOn = alarmBuzzerState;
  }
  else if (reminderActive)
  {
    unsigned long elapsed = millis() - reminderCycleStart;
    if (elapsed >= REMINDER_PERIOD_MS)
    {
      reminderCycleStart += REMINDER_PERIOD_MS; // += để tránh trôi thời gian
      elapsed = millis() - reminderCycleStart;
    }
    buzzerOn = (elapsed < REMINDER_BEEP_MS);
  }

  // 3. Bíp đơn lẻ: chỉ hết hạn độc lập, không bị chặn bởi nhánh trên
  if (beepEndTime != 0)
  {
    if ((long)(millis() - beepEndTime) >= 0) beepEndTime = 0;
    else buzzerOn = true;
  }

  digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);

  // 4. Pattern chớp LED đỏ
  if (ledBlinkActive)
  {
    if (millis() - lastLedToggle >= LED_BLINK_MS)
    {
      lastLedToggle = millis();
      ledBlinkState = !ledBlinkState;
      digitalWrite(LED_PIN, ledBlinkState ? HIGH : LOW);
    }
  }
}

// Còi
void beepOnce(unsigned long ms)
{
  beepEndTime = millis() + ms;
  if (!alarmActive && (!reminderActive || !reminderBuzzerState))
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void beepError()
{
  beepOnce(600);
}

void alarmStart()
{
  alarmActive = true;
  alarmBuzzerState = true;
  lastAlarmToggle = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}

void alarmStop()
{
  alarmActive = false;
  alarmBuzzerState = false;
  if (reminderActive)
  {
    reminderCycleStart = millis();
    reminderBuzzerState = true;
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (beepEndTime == 0)
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void reminderStart()
{
  reminderActive = true;
  reminderCycleStart = millis();
  reminderBuzzerState = true;
  if (!alarmActive)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void reminderStop()
{
  reminderActive = false;
  reminderBuzzerState = false;
  if (!alarmActive && beepEndTime == 0)
  {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// LED
void ledSet(bool on)
{
  ledBlinkActive = false;
  ledBlinkState = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void ledBlinkStart()
{
  ledBlinkActive = true;
  ledBlinkState = true;
  lastLedToggle = millis();
  digitalWrite(LED_PIN, HIGH);
}

void ledBlinkStop()
{
  ledBlinkActive = false;
  ledBlinkState = false;
  digitalWrite(LED_PIN, LOW);
}

// Công tắc cửa
bool doorIsOpen()
{
  return doorDebouncedState;
}
