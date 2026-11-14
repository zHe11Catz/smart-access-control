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

static bool currentBuzzerState = false;
static unsigned int currentBuzzerFreq = 0;

// Hàm điều khiển còi thụ động bằng tone() để phát âm thanh thật trên Wokwi
static void setBuzzerHardware(bool on, unsigned int freq = 2000)
{
  if (on)
  {
    if (!currentBuzzerState || currentBuzzerFreq != freq)
    {
      tone(BUZZER_PIN, freq);
      currentBuzzerState = true;
      currentBuzzerFreq = freq;
    }
  }
  else
  {
    if (currentBuzzerState)
    {
      noTone(BUZZER_PIN);
      currentBuzzerState = false;
      currentBuzzerFreq = 0;
    }
  }
}

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

  setBuzzerHardware(false);
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
  unsigned int buzzerFreq = 2000;

  if (alarmActive)
  {
    if (millis() - lastAlarmToggle >= ALARM_BEEP_MS)
    {
      lastAlarmToggle = millis();
      alarmBuzzerState = !alarmBuzzerState;
    }
    buzzerOn = alarmBuzzerState;
    buzzerFreq = 2500;
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
    buzzerFreq = 2000;
  }

  // 3. Bíp đơn lẻ: chỉ hết hạn độc lập, không bị chặn bởi nhánh trên
  if (beepEndTime != 0)
  {
    if ((long)(millis() - beepEndTime) >= 0) beepEndTime = 0;
    else
    {
      buzzerOn = true;
      if (!alarmActive) buzzerFreq = 2000;
    }
  }

  setBuzzerHardware(buzzerOn, buzzerFreq);

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
    setBuzzerHardware(true, 2000);
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
  setBuzzerHardware(true, 2500);
}

void alarmStop()
{
  alarmActive = false;
  alarmBuzzerState = false;
  if (reminderActive)
  {
    reminderCycleStart = millis();
    reminderBuzzerState = true;
    setBuzzerHardware(true, 2000);
  }
  else if (beepEndTime == 0)
  {
    setBuzzerHardware(false);
  }
}

void reminderStart()
{
  reminderActive = true;
  reminderCycleStart = millis();
  reminderBuzzerState = true;
  if (!alarmActive)
  {
    setBuzzerHardware(true, 2000);
  }
}

void reminderStop()
{
  reminderActive = false;
  reminderBuzzerState = false;
  if (!alarmActive && beepEndTime == 0)
  {
    setBuzzerHardware(false);
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
