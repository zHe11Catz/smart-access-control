#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

#define SS_PIN      5
#define RST_PIN     27
#define SERVO_PIN   13
#define BUZZER_PIN  15
#define LED_PIN     2
#define DOOR_PIN    4

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

bool opened = false;
int lastDoor = -1;

void showOled(const String& l1, const String& l2 = "") {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);  oled.println(l1);
  oled.setCursor(0, 20); oled.println(l2);
  oled.display();
}

void beep(int ms) {
  digitalWrite(BUZZER_PIN, HIGH); delay(ms); digitalWrite(BUZZER_PIN, LOW);
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
  lockServo.write(0);

  showOled("Smart Access", "Test phan cung");
  Serial.println("San sang. Thu: phim, the, cong tac cua.");
}

void loop() {
  // 1) Keypad
  char k = keypad.getKey();
  if (k) {
    Serial.print("Phim: "); Serial.println(k);
    showOled("Phim bam:", String(k));
    beep(60);
    if (k == '#') {                 // '#' de thu Servo + LED
      opened = !opened;
      lockServo.write(opened ? 90 : 0);
      digitalWrite(LED_PIN, opened);
    }
  }

  // 2) RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
      if (i < rfid.uid.size - 1) uid += " ";
    }
    uid.toUpperCase();
    Serial.println("UID: " + uid);
    showOled("The RFID:", uid);
    rfid.PICC_HaltA();
  }

  // 3) Cong tac cua
  int d = digitalRead(DOOR_PIN);
  if (d != lastDoor) {
    lastDoor = d;
    Serial.println(d == LOW ? "Cua: DONG (LOW)" : "Cua: MO (HIGH)");
  }
}