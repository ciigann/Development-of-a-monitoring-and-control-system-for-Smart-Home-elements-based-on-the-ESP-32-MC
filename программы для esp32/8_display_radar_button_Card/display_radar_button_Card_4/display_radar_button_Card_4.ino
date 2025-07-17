#include <Arduino.h>
#include <LD2450.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <vector>
#include "FS.h"
#include "SD.h"

// Определение серого цвета
#define ST77XX_GRAY 0x8C51  // Пример определения серого цвета

// Определение пинов для подключения SD-карты
#define SD_CS 33
#define SD_SCLK 25
#define SD_MOSI 26
#define SD_MISO 27

// Объявление пинов для дисплея ST7789V2
#define TFT_CS     5   // CS
#define TFT_RST    4   // RST
#define TFT_DC     15  // DC (RS)
#define TFT_MOSI   19  // MOSI
#define TFT_SCLK   18  // SCLK

// Пин для кнопки Boot
#define BOOT_BUTTON_PIN 0

// Пин для светодиода
#define LED_PIN 2

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
const unsigned long doublePressDelay = 500; // Задержка для определения двойного нажатия
const unsigned long longPressDelay = 2000; // Задержка для определения длинного нажатия (2 секунды)

// Переменные для отслеживания состояния отображения и количества точек
bool displayNewZone = true;
int pointCount = 1;
bool zoneCreationMode = false;
unsigned long zoneCreationStartTime = 0;
int countdown = 5; // Начальное значение отсчета
bool showCheckmark = false;
unsigned long checkmarkStartTime = 0;

// Переменные для хранения предыдущих координат и состояния отображения
int prevX[3] = {-1, -1, -1}, prevY[3] = {-1, -1, -1};
bool prevDisplayNewZone = true;
int prevPointCount = 1;
int prevTargetX = 0, prevTargetY = 0;
bool prevZoneCreationMode = false;
int prevCountdown = 5;
bool prevShowCheckmark = false;

// Динамические массивы для хранения координат красных точек
std::vector<int> redPointsX;
std::vector<int> redPointsY;

// Флаг для отслеживания сохранения зоны
bool zoneSaved = false;

// Вектор для хранения зон
std::vector<std::vector<std::pair<int, int>>> zones;

// Флаг для отслеживания наличия SD-карты
bool sdCardPresent = true;

// Количество зон на карте памяти
int ZoneCount = 0;

// Флаг для отслеживания необходимости обновления меню
bool menuNeedsUpdate = true;

// Переменная для отслеживания текущей выбранной зоны
int selectedZone = -1;

void IRAM_ATTR handleButtonPress() {
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    buttonPressStartTime = millis();
    buttonPressed = true;
    buttonHandled = false;
  } else {
    if (buttonPressed && !buttonHandled && (millis() - lastPressTime > debounceDelay)) {
      if (millis() - lastPressTime < doublePressDelay) {
        Serial.println("Double press");
        if (setupMode && !displayNewZone) {
          if (zoneCreationMode) {
            zoneCreationMode = false;
          } else {
            zoneCreationMode = true;
            zoneCreationStartTime = millis();
            countdown = 5; // Сбрасываем отсчет
            // Очищаем предыдущие точки при входе в режим создания зоны
            redPointsX.clear();
            redPointsY.clear();
          }
        } else if (setupMode && displayNewZone) {
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

  // Инициализация светодиода
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Инициализация SD-карты
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("Card Mount Failed");
    sdCardPresent = false;
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(50, 100);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print("No SD card");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    sdCardPresent = false;
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(50, 100);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print("No SD card");
    return;
  }

  // Чтение зон из файла на SD-карте
  readZonesFromSD();

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

void drawCheckmark() {
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int size = 50; // Размер галочки

  // Включаем светодиод
  digitalWrite(LED_PIN, HIGH);

  // Рисование жирной галочки
  for (int i = -2; i <= 2; i++) {
    tft.drawLine(centerX - size / 2, centerY + i, centerX, centerY + size / 2 + i, ST77XX_GREEN);
    tft.drawLine(centerX, centerY + size / 2 + i, centerX + size, centerY - size + i, ST77XX_GREEN);
  }
}

void eraseCheckmark() {
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int size = 150; // Увеличенный размер области стирания

  // Стирание галочки
  tft.fillRect(centerX - size / 2, centerY - size / 2, size, size, ST77XX_BLACK);

  // Выключаем светодиод
  digitalWrite(LED_PIN, LOW);
}

void saveZoneToSD() {
  if (!sdCardPresent) {
    return;
  }

  File file = SD.open("/src/zone.txt", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Записываем координаты точек зоны в файл
  for (size_t i = 0; i < redPointsX.size(); i++) {
    if (i > 0) {
      file.print(";");
    }
    file.print(redPointsX[i]);
    file.print(",");
    file.print(redPointsY[i]);
  }
  file.println(); // Переход на новую строку для следующей зоны

  file.close();
  Serial.println("Zone coordinates saved to SD card");
  ZoneCount++; // Увеличиваем количество зон после сохранения
}

void readZonesFromSD() {
  if (!sdCardPresent) {
    return;
  }

  File file = SD.open("/src/zone.txt");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      std::vector<std::pair<int, int>> zone;
      int start = 0;
      int end = line.indexOf(';');
      while (end != -1) {
        String point = line.substring(start, end);
        int comma = point.indexOf(',');
        if (comma != -1) {
          int x = point.substring(0, comma).toInt();
          int y = point.substring(comma + 1).toInt();
          zone.push_back(std::make_pair(x, y));
        }
        start = end + 1;
        end = line.indexOf(';', start);
      }
      // Обработка последней точки в строке
      String point = line.substring(start);
      int comma = point.indexOf(',');
      if (comma != -1) {
        int x = point.substring(0, comma).toInt();
        int y = point.substring(comma + 1).toInt();
        zone.push_back(std::make_pair(x, y));
      }
      zones.push_back(zone);
    }
  }

  file.close();
  Serial.println("Zones read from SD card");
  ZoneCount = zones.size(); // Устанавливаем количество зон
}

void drawZones(int selectedZone = -1) {
  for (size_t i = 0; i < zones.size(); i++) {
    uint16_t color;
    if (setupMode) {
      color = (i + 1 == selectedZone) ? ST77XX_RED : ST77XX_GRAY; // Красный для выбранной зоны, серый для остальных в режиме настройки
    } else {
      color = ST77XX_RED; // Красный для всех зон вне режима настройки
    }
    for (size_t j = 0; j < zones[i].size(); j++) {
      int x = zones[i][j].first;
      int y = zones[i][j].second;
      tft.fillCircle(x, y, 5, color);
      if (j > 0) {
        int prevX = zones[i][j - 1].first;
        int prevY = zones[i][j - 1].second;
        tft.drawLine(prevX, prevY, x, y, color);
      }
    }
    // Соединение последней и первой точки зоны
    if (zones[i].size() > 1) {
      int firstX = zones[i][0].first;
      int firstY = zones[i][0].second;
      int lastX = zones[i][zones[i].size() - 1].first;
      int lastY = zones[i][zones[i].size() - 1].second;
      tft.drawLine(lastX, lastY, firstX, firstY, color);
    }
  }
}

void displayZonesInMenu() {
  tft.fillRect(tft.width() - 100, tft.height() - 40 - (ZoneCount + 1) * 20, 100, (ZoneCount + 1) * 20, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);

  // Вывод зон в столбик
  for (int i = ZoneCount; i > 0; i--) {
    tft.setCursor(tft.width() - 100, tft.height() - 40 - (ZoneCount - i + 1) * 20);
    if (selectedZone == i) {
      tft.setTextSize(2); // Увеличиваем размер шрифта для жирного отображения
    } else {
      tft.setTextSize(1); // Уменьшаем размер шрифта для обычного отображения
    }
    tft.print("zone ");
    tft.print(i);
  }

  // Очищаем область перед выводом "new zone"
  tft.fillRect(tft.width() - 100, tft.height() - 40, 100, 20, ST77XX_BLACK);

  // Вывод "new zone" внизу
  tft.setCursor(tft.width() - 100, tft.height() - 40);
  if (selectedZone == -1) {
    tft.setTextSize(2); // Увеличиваем размер шрифта для жирного отображения
  } else {
    tft.setTextSize(1); // Уменьшаем размер шрифта для обычного отображения
  }
  tft.print("new zone");
}

void clearMenu() {
  tft.fillRect(tft.width() - 100, tft.height() - 40 - (ZoneCount + 1) * 20, 100, (ZoneCount + 1) * 20 + 50, ST77XX_BLACK);
}

void loop() {
  // Проверяем, не прошла ли секунда с момента поднятия флага
  if (flagRaised && (millis() - flagRaiseTime >= doublePressDelay)) {
    flagRaised = false;
    Serial.println("Short press");
    if (setupMode) {
      if (displayNewZone && selectedZone == -1) {
        // Если "new zone" выбрана и нажата, делаем "new zone" не жирным и выбираем zone 1
        tft.fillRect(tft.width() - 100, tft.height() - 40, 100, 20, ST77XX_BLACK); // Стираем жирную надпись "new zone"
        selectedZone = 1;
        menuNeedsUpdate = true;
      } else if (!displayNewZone) {
        pointCount++;
        Serial.print("Point count: ");
        Serial.println(pointCount);
      } else {
        // Переключаемся на следующую зону
        if (selectedZone == ZoneCount) {
          // Если текущая зона последняя, выбираем "new zone"
          selectedZone = -1;
        } else {
          selectedZone = (selectedZone % ZoneCount) + 1;
        }
        menuNeedsUpdate = true;
      }
    }
  }

  // Если кнопка удерживается более 2 секунд
  if (buttonPressed && (millis() - buttonPressStartTime >= longPressDelay)) {
    setupMode = !setupMode; // Переключаем режим настройки
    if (setupMode) {
      Serial.println("Entering setup mode");
      menuNeedsUpdate = true; // Устанавливаем флаг для обновления меню
    } else {
      Serial.println("Exiting setup mode");
      displayNewZone = true; // Сбрасываем состояние отображения
      pointCount = 1; // Сбрасываем количество точек
      zoneCreationMode = false; // Сбрасываем режим создания зоны
      clearMenu(); // Очищаем меню
      selectedZone = -1; // Сбрасываем выбранную зону
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

        // Стираем предыдущий круг, если координаты изменились и это не красная точка
        bool isRedPoint = false;
        for (size_t j = 0; j < redPointsX.size(); j++) {
          if (redPointsX[j] == prevX[i] && redPointsY[j] == prevY[i]) {
            isRedPoint = true;
            break;
          }
        }
        if (prevX[i] != -1 && prevY[i] != -1 && !isRedPoint) {
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
          // Вывод надписи "new zone" или "n dot: Lc" жирным шрифтом
          if (menuNeedsUpdate) {
            displayZonesInMenu();
            menuNeedsUpdate = false;
          }

          tft.setCursor(tft.width() - 100, tft.height() - 40);
          tft.setTextColor(ST77XX_WHITE);
          tft.setTextSize(2); // Увеличиваем размер шрифта для жирного отображения

          if (displayNewZone) {
            // Надпись "new zone" уже выведена в displayZonesInMenu
          } else {
            // Стираем надпись "new zone"
            tft.fillRect(tft.width() - 100, tft.height() - 40, 100, 20, ST77XX_BLACK);

            if (zoneCreationMode) {
              if (pointCount == 1 && countdown == 1) {
                tft.print("new zone");
              } else {
                tft.print(pointCount);
                tft.print("dot: ");
                tft.print(countdown);
                tft.print("c");
              }
            } else {
              tft.print(pointCount);
              tft.print(" point");
            }
          }

          // Обновляем предыдущие состояния
          prevDisplayNewZone = displayNewZone;
          prevPointCount = pointCount;
          prevZoneCreationMode = zoneCreationMode;
          prevCountdown = countdown;
          prevShowCheckmark = showCheckmark;
        } else {
          clearMenu(); // Очищаем меню
          tft.print(" ");
        }
      }
    }
  }

  // Проверяем, не прошло ли 1 секунда с момента последнего обновления отсчета
  static unsigned long lastCountdownUpdate = 0;
  if (zoneCreationMode && (millis() - lastCountdownUpdate >= 1000)) {
    lastCountdownUpdate = millis();
    countdown--;
    if (countdown <= 0) {
      countdown = 5;
      pointCount--;
      if (pointCount <= 0) {
        pointCount = 1;
        zoneCreationMode = false;
        displayNewZone = true;
        menuNeedsUpdate = true; // Устанавливаем флаг для обновления меню
      }
    }
    if (countdown == 1) {
      showCheckmark = true;
      checkmarkStartTime = millis();
      // Добавляем новые координаты в динамический массив
      redPointsX.push_back(prevX[0]);
      redPointsY.push_back(prevY[0]);
    }
  }

  // Проверяем, не прошла ли 1 секунда с момента отображения галочки
  if (showCheckmark && (millis() - checkmarkStartTime >= 1000)) {
    showCheckmark = false;
    eraseCheckmark();
  }

  // Если нужно отобразить галочку
  if (showCheckmark) {
    drawCheckmark();
  }

  // Рисование всех красных точек
  for (size_t i = 0; i < redPointsX.size(); i++) {
    if (redPointsX[i] != -1 && redPointsY[i] != -1) {
      tft.fillCircle(redPointsX[i], redPointsY[i], 5, ST77XX_RED);
    }
  }

  // Соединение красных точек линиями (если есть хотя бы 2 точки)
  if (redPointsX.size() >= 2) {
    for (size_t i = 0; i < redPointsX.size(); i++) {
      size_t next_i = (i + 1) % redPointsX.size(); // Следующая точка (для последней точки - первая)

      // Проверяем, что обе точки валидны
      if (redPointsX[i] != -1 && redPointsY[i] != -1 &&
          redPointsX[next_i] != -1 && redPointsY[next_i] != -1) {
        // Рисуем линию между текущей и следующей точкой
        tft.drawLine(redPointsX[i], redPointsY[i],
                    redPointsX[next_i], redPointsY[next_i], ST77XX_RED);
      }
    }
  }

  // Сохранение зоны на SD-карту, если pointCount = 1 и countdown = 1
  if (pointCount == 1 && countdown == 1 && !zoneSaved) {
    saveZoneToSD();
    zoneSaved = true;
    menuNeedsUpdate = true; // Устанавливаем флаг для обновления меню
  }

  // Сбрасываем флаг сохранения зоны, если режим создания зоны завершен
  if (!zoneCreationMode) {
    zoneSaved = false;
  }

  // Отрисовка зон
  drawZones(selectedZone);
}
