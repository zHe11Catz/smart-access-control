#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ===== Chân =====
#define SS_PIN      5
#define RST_PIN     27
#define SERVO_PIN   13
#define BUZZER_PIN  15
#define LED_PIN     2
#define DOOR_PIN    4

// ===== Hằng số =====
const int           SERVO_LOCK_ANGLE   = 0;
const int           SERVO_UNLOCK_ANGLE = 90;
const unsigned long UNLOCK_DURATION_MS = 5000;   // mở 5s rồi tự khóa

// ===== Đối tượng phần cứng =====
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Servo lockServo;

const byte ROWS = 4, COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'*','0','#'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {14, 16, 17};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===== FSM =====
enum State { STATE_LOCKED, STATE_UNLOCKED };
State state = STATE_LOCKED;
unsigned long stateEnterTime = 0;

const char* stateName(State s) {
  switch (s) {
    case STATE_LOCKED:   return "STATE_LOCKED";
    case STATE_UNLOCKED: return "STATE_UNLOCKED";
  }
  return "UNKNOWN";
}

// ===== Hiển thị =====
void showOled(const String& l1, const String& l2 = "") {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);  oled.println(l1);
  oled.setCursor(0, 20); oled.println(l2);
  oled.display();
}

// ===== Còi non-blocking =====
unsigned long beepEndTime = 0;

void beepStart(unsigned long ms) {
  digitalWrite(BUZZER_PIN, HIGH);
  beepEndTime = millis() + ms;
}

void beepUpdate() {
  if (beepEndTime != 0 && (long)(millis() - beepEndTime) >= 0) {
    digitalWrite(BUZZER_PIN, LOW);
    beepEndTime = 0;
  }
}

// ===== Chuyển trạng thái: nơi thay đổi state =====
void enterState(State next) {
  Serial.printf("[FSM] %s -> %s\r\n", stateName(state), stateName(next));
  state = next;
  stateEnterTime = millis();

  switch (state) {
    case STATE_LOCKED:
      lockServo.write(SERVO_LOCK_ANGLE);
      digitalWrite(LED_PIN, LOW);
      showOled("Smart Access", "Locked");
      break;

    case STATE_UNLOCKED:
      lockServo.write(SERVO_UNLOCK_ANGLE);
      digitalWrite(LED_PIN, HIGH);
      showOled("Smart Access", "Unlocked");
      beepStart(100);
      break;
  }
}

// ===== Sự kiện đầu vào tạm thời =====
// Trả về true nếu có "yêu cầu mở cửa"
bool checkUnlockRequest() {
  char k = keypad.getKey();
  if (k) {
    Serial.printf("Phim: %c\r\n", k);
    if (k == '#') return true;
  }
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("Phat hien the RFID");
    rfid.PICC_HaltA();
    return true;
  }
  return false;
}

// ===== Xử lý theo từng trạng thái =====
void handleLocked() {
  if (checkUnlockRequest()) {
    enterState(STATE_UNLOCKED);
  }
}

void handleUnlocked() {
  if (millis() - stateEnterTime > UNLOCK_DURATION_MS) {
    enterState(STATE_LOCKED);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(DOOR_PIN, INPUT_PULLUP);

  SPI.begin();
  rfid.PCD_Init();

  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  lockServo.attach(SERVO_PIN);

  enterState(STATE_LOCKED); // trạng thái khởi đầu
}

void loop() {
  beepUpdate();

  switch (state) {
    case STATE_LOCKED:   handleLocked();   break;
    case STATE_UNLOCKED: handleUnlocked(); break;
  }
}