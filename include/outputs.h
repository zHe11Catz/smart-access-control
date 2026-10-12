#pragma once
#include <Arduino.h>

void outputsInit();

// ----- Servo -----
void lockDoor();
void unlockDoor();

// ----- OLED: mỗi trạng thái 1 hàm hiển thị -----
void showOled(const String& l1, const String& l2 = ""); // hàm nền
void showLocked();
void showUnlocked();
void showPinInput(uint8_t len);        // hiện len dấu '*'
void showLockout(uint8_t secondsLeft); // 'LOCKED 30s'
void showError(const char* msg);       // 'Sai ma', 'The khong hop le'...
void showTamper();
void showDoorAjar();
