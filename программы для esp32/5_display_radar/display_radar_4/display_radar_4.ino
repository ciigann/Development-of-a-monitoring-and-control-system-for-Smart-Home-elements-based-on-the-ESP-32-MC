#include <Arduino.h>
#include <LD2450.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Объявление пинов для дисплея ST7789V2
#define TFT_CS     5   // CS
#define TFT_RST    4   // RST
#define TFT_DC     15  // DC (RS)
#define TFT_MOSI   19  // MOSI
#define TFT_SCLK   18  // SCLK

// Пин для кнопки Boot
#define BOOT_BUTTON_PIN 0

// Создаем объект для работы с дисплеем
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Создаем объект для работы с лидаром
LD2450 ld2450;

// Флаг для режима настройки
bool setupMode = false;

// Переменные для отслеживания длительности нажатия кнопки
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(115200);

  // Настраиваем пин кнопки Boot как вход с подтяжкой вверх
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Инициализация последовательного порта для отладки
  Serial1.begin(256000, SERIAL_8N1, 16, 17);

  while (!Serial) {
    Serial.println("wait for serial port to connect");
  }

  while (!Serial1) {
    Serial.println("wait for serial1 port to connect");
  }

  // Инициализация дисплея
  tft.init(240, 320); // Инициализация с размером 240x320
  tft.setRotation(1);  // Установите ориентацию дисплея по необходимости
  tft.fillScreen(ST77XX_BLACK);

  // Настройка лидара
  byte enConfig[LD2450_SERIAL_BUFFER] = "";
  enConfig[0] = 0xFD;
  enConfig[1] = 0xFC;
  enConfig[2] = 0xFB;
  enConfig[3] = 0xFA;
  enConfig[4] = 0x04;
  enConfig[5] = 0x00;
  enConfig[6] = 0xFF;
  enConfig[7] = 0x00;
  enConfig[8] = 0x01;
  enConfig[9] = 0x00;
  enConfig[10] = 0x04;
  enConfig[11] = 0x03;
  enConfig[12] = 0x02;
  enConfig[13] = 0x01;

  Serial1.write(enConfig, sizeof(enConfig));

  while (Serial1.available() <= 0) {
    Serial.print('.');  // send a capital A
    delay(1);
  }

  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    Serial.print(inChar);
  }

  byte endConfig[LD2450_SERIAL_BUFFER] = "";
  endConfig[0] = 0xFD;
  endConfig[1] = 0xFC;
  endConfig[2] = 0xFB;
  endConfig[3] = 0xFA;
  endConfig[4] = 0x02;
  endConfig[5] = 0x00;
  endConfig[6] = 0xFE;
  endConfig[7] = 0x00;
  endConfig[8] = 0x04;
  endConfig[9] = 0x03;
  endConfig[10] = 0x02;
  endConfig[11] = 0x01;

  Serial1.write(endConfig, sizeof(endConfig));

  while (Serial1.available() <= 0) {
    Serial.print('.');  // send a capital A
    delay(1);
  }

  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    Serial.print(inChar);
  }

  ld2450.begin(Serial1, true);

  if (!ld2450.waitForSensorMessage()) {
    Serial.println("SENSOR CONNECTION SEEMS OK");
  } else {
    Serial.println("SENSOR TEST: GOT NO VALID SENSORDATA - PLEASE CHECK CONNECTION!");
  }
}

void loop() {
  // Считываем состояние кнопки Boot
  int bootButtonState = digitalRead(BOOT_BUTTON_PIN);

  // Если кнопка нажата
  if (bootButtonState == LOW) {
    if (!buttonPressed) {
      buttonPressStartTime = millis();
      buttonPressed = true;
    }

    // Если кнопка удерживается более 2 секунд
    if (buttonPressed && (millis() - buttonPressStartTime >= 2000)) {
      setupMode = !setupMode; // Переключаем режим настройки
      if (setupMode) {
        Serial.println("Entering setup mode");
      } else {
        Serial.println("Exiting setup mode");
      }
      buttonPressed = false; // Сбрасываем флаг нажатия
    }
  } else {
    buttonPressed = false; // Сбрасываем флаг нажатия, если кнопка отпущена
  }

  const int sensor_got_valid_targets = ld2450.read();
  if (sensor_got_valid_targets > 0) {
    tft.fillScreen(ST77XX_BLACK); // Очистка экрана

    for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++) {
      const LD2450::RadarTarget result_target = ld2450.getTarget(i);
      if (result_target.valid) {
        // Преобразование координат из миллиметров в пиксели с изменением знака X
        int x = map(result_target.x, -6000, 6000, tft.width(), 0);
        int y = map(result_target.y, 0, 6000, tft.height(), 0);

        // Рисование круга на дисплее
        tft.fillCircle(x, y, 5, ST77XX_WHITE); // Размер круга - 5 пикселей

        // Вывод координат в нижнем левом углу
        tft.setCursor(0, tft.height() - 20);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(1);
        tft.print("X: ");
        tft.print(result_target.x);
        tft.print("mm, Y: ");
        tft.print(result_target.y);
        tft.print("mm");

        // Вывод надписи в правом нижнем углу
        tft.setCursor(tft.width() - 100, tft.height() - 20);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(1);
        if (setupMode) {
          tft.print("setting zones");
          // Вывод надписи "new zone" жирным шрифтом
          tft.setCursor(tft.width() - 100, tft.height() - 40);
          tft.setTextColor(ST77XX_WHITE);
          tft.setTextSize(2); // Увеличиваем размер шрифта для жирного отображения
          tft.print("new zone");
        } else {
          tft.print(" ");
        }
      }
    }
  }
}
