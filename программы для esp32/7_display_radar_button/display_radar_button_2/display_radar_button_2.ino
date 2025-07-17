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

// Переменные для отслеживания состояния кнопки
volatile unsigned long buttonPressStartTime = 0;
volatile bool buttonPressed = false;
volatile bool buttonHandled = false;
volatile unsigned long lastPressTime = 0;
volatile bool flagRaised = false;
volatile unsigned long flagRaiseTime = 0;
const unsigned long debounceDelay = 200; // Задержка для устранения дребезга
const unsigned long doublePressDelay = 400; // Задержка для определения двойного нажатия
const unsigned long longPressDelay = 1000; // Задержка для определения длинного нажатия

// Переменные для отслеживания состояния отображения и количества точек
bool displayNewZone = true;
int pointCount = 1;

// Переменные для хранения предыдущих координат и состояния отображения
int prevX[3] = {-1, -1, -1}, prevY[3] = {-1, -1, -1};
bool prevDisplayNewZone = true;
int prevPointCount = 1;
int prevTargetX = 0, prevTargetY = 0;

void IRAM_ATTR handleButtonPress() {
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    buttonPressStartTime = millis();
    buttonPressed = true;
    buttonHandled = false;
  } else {
    if (buttonPressed && !buttonHandled && (millis() - lastPressTime > debounceDelay)) {
      if (millis() - lastPressTime < doublePressDelay) {
        Serial.println("Double press");
        if (setupMode && displayNewZone) {
          displayNewZone = false;
        }
        flagRaised = false; // Сбрасываем флаг, чтобы не выводить "Short press"
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
}

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(115200);

  // Настраиваем пин кнопки Boot как вход с подтяжкой вверх
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Настройка прерывания для кнопки Boot
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), handleButtonPress, CHANGE);

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
  // Проверяем, не прошла ли секунда с момента поднятия флага
  if (flagRaised && (millis() - flagRaiseTime >= doublePressDelay)) {
    flagRaised = false;
    Serial.println("Short press");
    if (setupMode && !displayNewZone) {
      pointCount++;
      Serial.print("Point count: ");
      Serial.println(pointCount);
    }
  }

  // Если кнопка удерживается более 2 секунд
  if (buttonPressed && (millis() - buttonPressStartTime >= longPressDelay)) {
    setupMode = !setupMode; // Переключаем режим настройки
    if (setupMode) {
      Serial.println("Entering setup mode");
    } else {
      Serial.println("Exiting setup mode");
      displayNewZone = true; // Сбрасываем состояние отображения
      pointCount = 1; // Сбрасываем количество точек
    }
    buttonPressed = false; // Сбрасываем флаг нажатия
  }

  const int sensor_got_valid_targets = ld2450.read();
  if (sensor_got_valid_targets > 0) {
    for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++) {
      const LD2450::RadarTarget result_target = ld2450.getTarget(i);
      if (result_target.valid) {
        // Преобразование координат из миллиметров в пиксели с изменением знака X
        int x = map(result_target.x, -6000, 6000, tft.width(), 0);
        int y = map(result_target.y, 0, 6000, tft.height(), 0);

        // Стираем предыдущий круг, если координаты изменились
        if (prevX[i] != -1 && prevY[i] != -1) {
          tft.fillCircle(prevX[i], prevY[i], 5, ST77XX_BLACK);
        }

        // Рисование круга на дисплее
        tft.fillCircle(x, y, 5, ST77XX_WHITE); // Размер круга - 5 пикселей

        // Обновляем предыдущие координаты
        prevX[i] = x;
        prevY[i] = y;

        // Вывод координат в нижнем левом углу только для первой цели
        if (i == 0) {
          // Стираем предыдущие координаты
          tft.setCursor(0, tft.height() - 20);
          tft.setTextColor(ST77XX_BLACK);
          tft.setTextSize(1);
          tft.print("X: ");
          tft.print(prevTargetX);
          tft.print("mm, Y: ");
          tft.print(prevTargetY);
          tft.print("mm");

          // Вывод координат в нижнем левом углу
          tft.setCursor(0, tft.height() - 20);
          tft.setTextColor(ST77XX_WHITE);
          tft.setTextSize(1);
          tft.print("X: ");
          tft.print(result_target.x);
          tft.print("mm, Y: ");
          tft.print(result_target.y);
          tft.print("mm");

          // Обновляем предыдущие координаты
          prevTargetX = result_target.x;
          prevTargetY = result_target.y;
        }

        // Вывод надписи в правом нижнем углу
        tft.setCursor(tft.width() - 100, tft.height() - 20);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(1);
        if (setupMode) {
          tft.print("setting zones");
          // Вывод надписи "new zone" или "n point" жирным шрифтом
          tft.setCursor(tft.width() - 100, tft.height() - 40);
          tft.setTextColor(ST77XX_WHITE);
          tft.setTextSize(2); // Увеличиваем размер шрифта для жирного отображения

          // Стираем предыдущую надпись, если состояние изменилось
          if (prevDisplayNewZone != displayNewZone || prevPointCount != pointCount) {
            tft.fillRect(tft.width() - 100, tft.height() - 40, 100, 20, ST77XX_BLACK);
          }

          if (displayNewZone) {
            tft.print("new zone");
          } else {
            tft.print(pointCount);
            tft.print(" point");
          }

          // Обновляем предыдущие состояния
          prevDisplayNewZone = displayNewZone;
          prevPointCount = pointCount;
        } else {
          tft.fillRect(tft.width() - 100, tft.height() - 40, 100, 20, ST77XX_BLACK);
          tft.fillRect(tft.width() - 100, tft.height() - 20, 100, 20, ST77XX_BLACK);
          tft.print(" ");
        }
      }
    }
  }
}
