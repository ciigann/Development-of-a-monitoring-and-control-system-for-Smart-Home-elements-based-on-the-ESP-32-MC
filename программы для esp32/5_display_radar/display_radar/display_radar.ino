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

// Создаем объект для работы с дисплеем
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Создаем объект для работы с лидаром
LD2450 ld2450;

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(115200);
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
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Initializing...");

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

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.println("Setup finished");
}

void loop() {
  const int sensor_got_valid_targets = ld2450.read();
  if (sensor_got_valid_targets > 0) {
    tft.fillScreen(ST77XX_BLACK); // Очистка экрана
    tft.setCursor(0, 0);
    tft.println("Target detected:");

    for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++) {
      const LD2450::RadarTarget result_target = ld2450.getTarget(i);
      if (result_target.valid) {
        tft.print("ID=");
        tft.print(i);
        tft.print(" X=");
        tft.print(result_target.x);
        tft.print("mm, Y=");
        tft.print(result_target.y);
        tft.print("mm, SPEED=");
        tft.print(result_target.speed);
        tft.println("cm/s");
      }
    }
  }
}
