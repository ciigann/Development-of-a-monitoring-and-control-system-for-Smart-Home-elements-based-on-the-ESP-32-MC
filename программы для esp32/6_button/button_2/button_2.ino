#include <Arduino.h>

// Пин для кнопки Boot
#define BOOT_BUTTON_PIN 0

// Переменные для отслеживания состояния кнопки
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
bool buttonHandled = false;
unsigned long lastPressTime = 0;
bool flagRaised = false;
unsigned long flagRaiseTime = 0;
const unsigned long debounceDelay = 200; // Задержка для устранения дребезга
const unsigned long doublePressDelay = 1000; // Задержка для определения двойного нажатия
const unsigned long flagRaiseDuration = 500; // Длительность поднятия флага

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(115200);

  // Настраиваем пин кнопки Boot как вход с подтяжкой вверх
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // Считываем состояние кнопки Boot
  int bootButtonState = digitalRead(BOOT_BUTTON_PIN);

  // Если кнопка нажата
  if (bootButtonState == LOW) {
    if (!buttonPressed) {
      buttonPressStartTime = millis();
      buttonPressed = true;
      buttonHandled = false;
    }
  } else {
    if (buttonPressed && !buttonHandled && (millis() - lastPressTime > debounceDelay)) {
      if (millis() - lastPressTime < doublePressDelay) {
        Serial.println("Double press");
        flagRaised = false; // Сбрасываем флаг, чтобы не выводить "hort press"
      } else {
        if (!flagRaised) {
          flagRaised = true;
          flagRaiseTime = millis();
        }
      }
      buttonHandled = true;
      lastPressTime = millis();
    }
    buttonPressed = false; // Сбрасываем флаг нажатия, если кнопка отпущена
  }

  // Проверяем, не прошла ли секунда с момента поднятия флага
  if (flagRaised && (millis() - flagRaiseTime >= flagRaiseDuration)) {
    flagRaised = false;
    Serial.println("hort press");
  }
}
