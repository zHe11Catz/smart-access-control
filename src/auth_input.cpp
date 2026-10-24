#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include "config.h"
#include "auth_input.h"

// ---------- RFID ----------
static MFRC522 rfid(SS_PIN, RST_PIN);
static String  cardUid = "";

// UID thẻ hợp lệ
static const char* const VALID_UIDS[] = {
  "55 66 77 88",
};
static const size_t VALID_UID_COUNT = sizeof(VALID_UIDS) / sizeof(VALID_UIDS[0]);

static bool isValidUid(const String& uid) {
  for (size_t i = 0; i < VALID_UID_COUNT; i++) {
    if (uid == VALID_UIDS[i]) return true;
  }
  return false;
}

// ---------- Keypad ----------
static const byte ROWS = 4, COLS = 3;
static char keys[ROWS][COLS] = {
  {'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'*','0','#'}
};
static byte rowPins[ROWS] = {32, 33, 25, 26};
static byte colPins[COLS] = {14, 16, 17};
static Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------- Buffer PIN ----------
static const uint8_t PIN_MAX_LEN = 8;
static char          pinBuf[PIN_MAX_LEN + 1];
static uint8_t       pinLen = 0;
static unsigned long lastKeyTime = 0;

void pinClear() {
  pinLen = 0;
  pinBuf[0] = '\0';
  lastKeyTime = millis();
}

bool pinAppend(char c) {
  if (pinLen >= PIN_MAX_LEN) return false;
  pinBuf[pinLen++] = c;
  pinBuf[pinLen] = '\0';
  lastKeyTime = millis();
  return true;
}

uint8_t pinLength()            { return pinLen; }
bool    pinMatchesHome()       { return strcmp(pinBuf, HOME_PIN) == 0; }
unsigned long pinLastKeyTime() { return lastKeyTime; }
const String& lastCardUid()    { return cardUid; }

// ---------- Khởi tạo & polling ----------
void authInit() {
  SPI.begin();
  rfid.PCD_Init();
  pinClear();
}

InputEvent pollInput(char &key) {
  key = 0;

  char k = keypad.getKey();
  if (k) {
    key = k;
    Serial.printf("Phim: %c\r\n", k);
    if (k >= '0' && k <= '9') return EVT_KEY_DIGIT;
    if (k == '#')             return EVT_KEY_HASH;
    if (k == '*')             return EVT_KEY_STAR;
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
      if (i < rfid.uid.size - 1) uid += " ";
    }
    uid.toUpperCase();
    rfid.PICC_HaltA();
    cardUid = uid;
    Serial.println("UID: " + uid);
    return isValidUid(uid) ? EVT_CARD_VALID : EVT_CARD_INVALID;
  }

  return EVT_NONE;
}
