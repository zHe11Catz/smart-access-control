#pragma once
#include <Arduino.h>

void alertsInit();
void alertsUpdate(); // gọi mỗi vòng loop() để chạy các pattern còi/LED

// ----- Còi (mỗi hàm chỉ khởi động pattern, alertsUpdate() lo phần còn lại) -----
void beepOnce(unsigned long ms); // tít ngắn
void beepError();                // tít dài (sai thẻ / sai mã)
void alarmStart();               // hú báo động khẩn (tamper / lockout)
void alarmStop();
void reminderStart();            // bíp nhắc nhở quên đóng cửa
void reminderStop();

// ----- LED -----
void ledSet(bool on);
void ledBlinkStart();            // LED đỏ chớp khi báo động
void ledBlinkStop();

// ----- Công tắc cửa -----
bool doorIsOpen();               // true = cửa hở (đã chống dội)
