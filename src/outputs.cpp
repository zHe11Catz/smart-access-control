// ============================================================
//  outputs.cpp - Hiện thực điều khiển Servo và OLED SSD1306
//  Phụ trách bởi: Vy (Task B) - Nhánh feature/outputs
// ============================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include "config.h"
#include "outputs.h"

// ============================================================
//  Khởi tạo đối tượng phần cứng
// ============================================================
static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static Servo lockServo;
static bool oledReady = false;

// ============================================================
//  Hàm đồ họa phụ trợ nội bộ (Internal Graphic Helpers)
// ============================================================

// Căn giữa chuỗi ký tự theo trục X tại tọa độ y
static void drawCenteredText(const char* text, int y, uint8_t size = 1, uint16_t color = SSD1306_WHITE) {
  if (!text || !oledReady) return;
  oled.setTextSize(size);
  oled.setTextColor(color);
  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - (int)w) / 2;
  if (x < 0) x = 0;
  oled.setCursor(x, y);
  oled.print(text);
}

// Vẽ thanh tiêu đề chuẩn phía trên màn hình
static void drawHeader(const char* title, bool inverted = false) {
  if (!oledReady) return;
  if (inverted) {
    oled.fillRect(0, 0, 128, 13, SSD1306_WHITE);
    drawCenteredText(title, 3, 1, SSD1306_BLACK);
  } else {
    drawCenteredText(title, 2, 1, SSD1306_WHITE);
    oled.drawFastHLine(0, 12, 128, SSD1306_WHITE);
  }
}

// Vẽ thanh hướng dẫn dưới chân màn hình (Footer)
static void drawFooter(const char* text) {
  if (!oledReady) return;
  oled.drawFastHLine(0, 51, 128, SSD1306_WHITE);
  drawCenteredText(text, 54, 1, SSD1306_WHITE);
}

// Vẽ biểu tượng ổ khóa đóng (Locked Icon) kích thước 16x19 tại (x, y)
static void drawLockIcon(int x, int y) {
  if (!oledReady) return;
  // Quai khóa
  oled.drawRoundRect(x + 2, y, 12, 11, 4, SSD1306_WHITE);
  // Thân khóa
  oled.fillRoundRect(x, y + 8, 16, 11, 2, SSD1306_WHITE);
  // Lỗ khóa
  oled.fillCircle(x + 8, y + 12, 1, SSD1306_BLACK);
  oled.drawFastVLine(x + 8, y + 13, 3, SSD1306_BLACK);
}

// Vẽ biểu tượng ổ khóa mở (Unlocked Icon) kích thước 16x19 tại (x, y)
static void drawUnlockIcon(int x, int y) {
  if (!oledReady) return;
  // Quai khóa mở nhấc lên phía trên bên phải
  oled.drawRoundRect(x + 4, y - 3, 12, 11, 4, SSD1306_WHITE);
  oled.fillRect(x + 4, y + 5, 4, 4, SSD1306_BLACK); // Khoảng hở quai mở
  // Thân khóa
  oled.fillRoundRect(x, y + 8, 16, 11, 2, SSD1306_WHITE);
  // Lỗ khóa
  oled.fillCircle(x + 8, y + 12, 1, SSD1306_BLACK);
  oled.drawFastVLine(x + 8, y + 13, 3, SSD1306_BLACK);
}

// Vẽ biểu tượng tam giác cảnh báo kích thước 20x20 tại (x, y)
static void drawWarningIcon(int x, int y) {
  if (!oledReady) return;
  oled.drawTriangle(x + 10, y, x, y + 20, x + 20, y + 20, SSD1306_WHITE);
  oled.drawFastVLine(x + 10, y + 7, 7, SSD1306_WHITE);
  oled.drawPixel(x + 10, y + 17, SSD1306_WHITE);
}

// Vẽ biểu tượng dấu chéo tròn [X] (Error Icon) bán kính 9 tại tâm (cx, cy)
static void drawErrorIcon(int cx, int cy) {
  if (!oledReady) return;
  oled.drawCircle(cx, cy, 9, SSD1306_WHITE);
  oled.drawLine(cx - 4, cy - 4, cx + 4, cy + 4, SSD1306_WHITE);
  oled.drawLine(cx - 4, cy + 4, cx + 4, cy - 4, SSD1306_WHITE);
}

// ============================================================
//  Khởi tạo phần cứng Output (Outputs Initialization)
// ============================================================
void outputsInit() {
  // 1. Khởi tạo bus I2C (SDA=21, SCL=22) và kiểm tra màn hình OLED SSD1306
  Wire.begin(21, 22);
  oledReady = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledReady) {
    Serial.println(F("[OUTPUTS] Khoi tao SSD1306 that bai! Kiem tra ket noi I2C."));
  } else {
    Serial.println(F("[OUTPUTS] OLED SSD1306 khoi tao thanh cong."));
    oled.clearDisplay();
    oled.display();
  }

  // 2. Cấp phát timer cho ESP32 và cấu hình tần số chuẩn 50Hz cho Servo SG90
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  lockServo.setPeriodHertz(50);
  lockServo.attach(SERVO_PIN, 500, 2400);

  // Mặc định chốt cửa ở trạng thái khóa
  lockDoor();
}

// ============================================================
//  Điều khiển Động cơ Servo (Chốt khóa cửa)
// ============================================================
void lockDoor() {
  lockServo.write(SERVO_LOCK_ANGLE);
  Serial.printf("[SERVO] Dong chot khoa (Goc: %d do)\r\n", SERVO_LOCK_ANGLE);
}

void unlockDoor() {
  lockServo.write(SERVO_UNLOCK_ANGLE);
  Serial.printf("[SERVO] Mo chot khoa (Goc: %d do)\r\n", SERVO_UNLOCK_ANGLE);
}

// ============================================================
//  Hiển thị Màn hình OLED SSD1306 (128x64)
// ============================================================

// Hàm nền hiển thị tùy ý 2 dòng
void showOled(const String& l1, const String& l2) {
  if (!oledReady) return;
  oled.clearDisplay();

  // Khung viền toàn màn hình
  oled.drawRoundRect(0, 0, 128, 64, 3, SSD1306_WHITE);
  oled.drawFastHLine(3, 14, 122, SSD1306_WHITE);

  // Dòng 1: Tiêu đề
  drawCenteredText(l1.c_str(), 4, 1, SSD1306_WHITE);

  // Dòng 2: Nội dung chính
  if (l2.length() > 0) {
    if (l2.length() <= 8) {
      drawCenteredText(l2.c_str(), 28, 2, SSD1306_WHITE);
    } else {
      drawCenteredText(l2.c_str(), 32, 1, SSD1306_WHITE);
    }
  }

  oled.display();
}

// 1. Màn hình LOCKED: Hệ thống đang khóa, chờ quẹt thẻ hoặc bấm PIN
void showLocked() {
  if (!oledReady) return;
  oled.clearDisplay();

  drawHeader("SMART ACCESS");

  // Icon ổ khóa đóng tại (18, 18)
  drawLockIcon(18, 18);

  // Text thông tin
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(44, 17);
  oled.print("LOCKED");

  oled.setTextSize(1);
  oled.setCursor(44, 35);
  oled.print("[Da khoa chot]");

  drawFooter("Quet the / Bam PIN");
  oled.display();
}

// 2. Màn hình UNLOCKED: Cửa đã mở thành công, chào đón người dùng
void showUnlocked() {
  if (!oledReady) return;
  oled.clearDisplay();

  drawHeader("SMART ACCESS");

  // Icon ổ khóa mở tại (18, 18)
  drawUnlockIcon(18, 18);

  // Text thông tin
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(44, 17);
  oled.print("OPEN");

  oled.setTextSize(1);
  oled.setCursor(44, 35);
  oled.print("Xin moi vao (5s)");

  drawFooter("Cua dang mo...");
  oled.display();
}

// 3. Màn hình PIN INPUT: Nhập mã số trên Keypad
void showPinInput(uint8_t len) {
  if (!oledReady) return;
  oled.clearDisplay();

  drawHeader("NHAP MA PIN");

  // Hộp khung hiển thị mã PIN
  oled.drawRoundRect(14, 18, 100, 26, 4, SSD1306_WHITE);

  // Hiển thị ký tự hoặc placeholder
  if (len == 0) {
    drawCenteredText("- - - -", 24, 2, SSD1306_WHITE);
  } else {
    String stars = "";
    if (len <= 4) {
      for (uint8_t i = 0; i < len; i++) {
        stars += "* ";
      }
      stars.trim();
    } else {
      if (len > 8) len = 8;
      for (uint8_t i = 0; i < len; i++) {
        stars += "*";
      }
    }
    drawCenteredText(stars.c_str(), 24, 2, SSD1306_WHITE);
  }

  drawFooter("#: OK    *: Xoa");
  oled.display();
}

// 4. Màn hình LOCKOUT: Tạm khóa bảo vệ an ninh do sai mã 3 lần
void showLockout(uint8_t secondsLeft) {
  if (!oledReady) return;
  oled.clearDisplay();

  // Banner cảnh báo đảo màu
  drawHeader("TAM KHOA AN NINH", true);

  // Icon tam giác cảnh báo
  drawWarningIcon(14, 20);

  // Đồng hồ đếm ngược
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(46, 18);
  if (secondsLeft < 10) oled.print("0");
  oled.print(secondsLeft);
  oled.print("s");

  oled.setTextSize(1);
  oled.setCursor(46, 36);
  oled.print("Sai ma 3 lan!");

  drawFooter("Vui long cho het gio");
  oled.display();
}

// 5. Màn hình ERROR: Báo lỗi quẹt thẻ hoặc mã PIN sai
void showError(const char* msg) {
  if (!oledReady) return;
  oled.clearDisplay();

  // Banner đảo màu lỗi
  drawHeader("XAC THUC THAT BAI", true);

  // Icon dấu X tròn
  drawErrorIcon(22, 32);

  // Nội dung lỗi
  const char* errorText = (msg && strlen(msg) > 0) ? msg : "Loi xac thuc";
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(40, 22);
  oled.print(errorText);

  oled.setCursor(40, 36);
  oled.print("Vui long thu lai");

  drawFooter("Quet the / Bam PIN");
  oled.display();
}

// 6. Màn hình TAMPER: Báo động khẩn khi bị cạy cửa trái phép
void showTamper() {
  if (!oledReady) return;
  oled.clearDisplay();

  // Tiêu đề cảnh báo khẩn cấp đảo màu
  drawHeader("! CANH BAO KHAN !", true);

  // Khung viền nổi bật
  oled.drawRect(0, 16, 128, 48, SSD1306_WHITE);

  drawCenteredText("CAY CUA!", 22, 2, SSD1306_WHITE);
  drawCenteredText("Phat hien dot nhap!", 40, 1, SSD1306_WHITE);
  drawCenteredText("Bao dong dang bat!", 52, 1, SSD1306_WHITE);

  oled.display();
}

// 7. Màn hình DOOR AJAR: Nhắc nhở người dùng đóng cửa
void showDoorAjar() {
  if (!oledReady) return;
  oled.clearDisplay();

  drawHeader("NHAC NHO AN TOAN");

  // Icon tam giác cảnh báo tại (12, 20)
  drawWarningIcon(12, 20);

  // Text thông tin bên phải (size 1 để không tràn màn hình 128px)
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(40, 22);
  oled.print("CUA CHUA DONG!");

  oled.setCursor(40, 35);
  oled.print("Cua mo qua 30s");

  drawFooter("Vui long khep cua lai");
  oled.display();
}
