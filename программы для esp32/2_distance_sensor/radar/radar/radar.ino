#include <Arduino.h>
#include <LD2450.h>

// Определяем контакты для подключения радара
#define RADAR_RX_PIN 23  // RX радара подключен к GPIO22
#define RADAR_TX_PIN 22  // TX радара подключен к GPIO23

// Встроенный светодиод для индикации обнаружения
const int ledPin = 2;

// Создаем экземпляр радара
LD2450 ld2450;

// Создаем аппаратный UART для ESP32
HardwareSerial radarSerial(1);  // Используем UART1 на ESP32

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(9600);
  while (!Serial) {
    ; // Ждем подключения последовательного порта (для некоторых плат)
  }
  
  Serial.println("Начало настройки LD2450...");

  // Настройка светодиода
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Инициализация UART для радара
  radarSerial.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  
  // Инициализация радара
  ld2450.begin(radarSerial, false);

  // Проверка подключения к радару
  if(!ld2450.waitForSensorMessage()) {
    Serial.println("Радар подключен успешно!");
  } else {
    Serial.println("Ошибка подключения к радару! Проверьте соединения.");
  }

  Serial.println("Настройка завершена, начинаем сканирование...");
}

void loop() {
  // Чтение данных с радара
  const int sensor_got_valid_targets = ld2450.read();
  
  if (sensor_got_valid_targets > 0) {
    // Выводим последнее сообщение от радара
    Serial.print(ld2450.getLastTargetMessage());

    // Проверяем все возможные цели
    for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++) {
      const LD2450::RadarTarget result_target = ld2450.getTarget(i);
      
      // Если цель валидна и движется
      if (result_target.valid && abs(result_target.speed) > 0) {
        Serial.println("Обнаружена движущаяся цель!");
        digitalWrite(ledPin, HIGH);
        
        // Дополнительная информация о цели
        Serial.print("Цель #");
        Serial.print(i);
        Serial.print(": X=");
        Serial.print(result_target.x);
        Serial.print("mm, Y=");
        Serial.print(result_target.y);
        Serial.print("mm, Скорость=");
        Serial.print(result_target.speed);
        Serial.println("cm/s");
      } else {
        digitalWrite(ledPin, LOW);
      }
    }
  } else if (sensor_got_valid_targets == -1) {
    Serial.println("Ошибка чтения данных радара!");
  } else if (sensor_got_valid_targets == -2) {
    Serial.println("Неверные данные от радара!");
  }
  
  delay(100); // Небольшая задержка для стабильности
}