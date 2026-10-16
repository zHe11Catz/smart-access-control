#pragma once
// ============================================================
//  outputs.h - Điều khiển Động cơ Servo và Màn hình OLED SSD1306
//  Phụ trách bởi: Vy (Task B) - Nhánh feature/outputs
// ============================================================
#include <Arduino.h>

void outputsInit();

// ----- Servo -----
void lockDoor();
void unlockDoor();

// ----- OLED: mỗi trạng thái 1 hàm hiển thị -----
void showOled(const String& l1, const String& l2 = ""); // hàm nền hiển thị 2 dòng
void showLocked();                                     // Màn hình trạng thái đã khóa
void showUnlocked();                                   // Màn hình trạng thái mở cửa
void showPinInput(uint8_t len);                        // Màn hình nhập mã PIN (hiện len dấu '*')
void showLockout(uint8_t secondsLeft);                 // Màn hình tạm khóa hệ thống khi sai quá số lần
void showError(const char* msg);                       // Màn hình thông báo lỗi xác thực
void showTamper();                                     // Màn hình báo động khẩn cấp khi bị cạy cửa
void showDoorAjar();                                   // Màn hình nhắc nhở quên đóng cửa
