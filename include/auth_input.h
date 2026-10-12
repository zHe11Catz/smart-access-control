#pragma once
#include <Arduino.h>

enum InputEvent {
  EVT_NONE,
  EVT_CARD_VALID,   // quẹt thẻ, UID nằm trong danh sách
  EVT_CARD_INVALID, // quẹt thẻ, UID không hợp lệ
  EVT_KEY_DIGIT,    // phím '0'-'9'
  EVT_KEY_HASH,     // phím '#'
  EVT_KEY_STAR      // phím '*'
};

void authInit();

// Gọi mỗi vòng loop(). Trả về 1 sự kiện hoặc EVT_NONE.
// key: ký tự phím vừa bấm (0 nếu không có phím)
InputEvent pollInput(char &key);

// UID của thẻ vừa quẹt, dùng cho log MQTT
const String& lastCardUid();

// ----- Buffer PIN -----
void pinClear();
bool pinAppend(char c);         // false nếu buffer đầy
uint8_t pinLength();
bool pinMatchesHome();          // so với HOME_PIN
unsigned long pinLastKeyTime(); // millis() của phím cuối, dùng cho timeout 10s
