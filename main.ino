#include <Arduino.h>
#include <LD2450.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <vector>
#include <FS.h>
#include <SD.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// --- Определение констант для цветов и пинов ---
// Цвета для дисплея
#define ST77XX_GRAY 0x8C51  // Серый цвет для отображения зон
// Пины для SD-карты
#define SD_CS 33
#define SD_SCLK 25
#define SD_MOSI 26
#define SD_MISO 27
// Пины для дисплея ST7789
#define TFT_CS 5
#define TFT_RST 4
#define TFT_DC 15
#define TFT_MOSI 19
#define TFT_SCLK 18
// Пин для кнопки Boot
#define BOOT_BUTTON_PIN 0
// Пин для светодиода
#define LED_PIN 2

// =============================================
// Класс Display: Управление экраном ST7789
// =============================================
class Display {
private:
    Adafruit_ST7789 tft;  // Объект для работы с дисплеем

public:
    // Конструктор: инициализация пинов дисплея
    Display() : tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST) {}

    // Инициализация дисплея: установка размера, ориентации и очистка экрана
    void init() {
        tft.init(240, 320);  // Размер экрана 240x320
        tft.setRotation(1);  // Установка ориентации
        tft.fillScreen(ST77XX_BLACK);  // Очистка экрана чёрным цветом
    }

    // Отрисовка иконки Wi-Fi в зависимости от состояния подключения
    void drawWiFiIcon(bool connected) {
        int x = 10, y = 10, size = 20;  // Координаты и размер иконки
        tft.fillRect(x - size, y - size, size * 2, size * 2, ST77XX_BLACK);  // Очистка области под иконкой
        if (connected) {
            // Рисуем подключённый Wi-Fi
            tft.drawCircle(x, y, size / 2, ST77XX_WHITE);
            tft.drawCircle(x, y, size / 4, ST77XX_WHITE);
            tft.drawLine(x, y - size / 2, x, y + size / 2, ST77XX_WHITE);
            tft.drawLine(x - size / 2, y, x + size / 2, y, ST77XX_WHITE);
        } else {
            // Рисуем зачёркнутый Wi-Fi (нет подключения)
            tft.drawCircle(x, y, size / 2, ST77XX_WHITE);
            tft.drawCircle(x, y, size / 4, ST77XX_WHITE);
            tft.drawLine(x, y - size / 2, x, y + size / 2, ST77XX_WHITE);
            tft.drawLine(x - size / 2, y, x + size / 2, y, ST77XX_WHITE);
            tft.drawLine(x - size / 2, y - size / 2, x + size / 2, y + size / 2, ST77XX_WHITE);
        }
    }

    // Отрисовка галочки (используется для подтверждения действий)
    void drawCheckmark() {
        int centerX = tft.width() / 2, centerY = tft.height() / 2, size = 50;
        digitalWrite(LED_PIN, HIGH);  // Включаем светодиод
        // Рисуем жирную зелёную галочку
        for (int i = -2; i <= 2; i++) {
            tft.drawLine(centerX - size / 2, centerY + i, centerX, centerY + size / 2 + i, ST77XX_GREEN);
            tft.drawLine(centerX, centerY + size / 2 + i, centerX + size, centerY - size + i, ST77XX_GREEN);
        }
    }

    // Удаление галочки с экрана
    void eraseCheckmark() {
        int centerX = tft.width() / 2, centerY = tft.height() / 2, size = 150;
        tft.fillRect(centerX - size / 2, centerY - size / 2, size, size, ST77XX_BLACK);  // Стираем область галочки
        digitalWrite(LED_PIN, LOW);  // Выключаем светодиод
    }

    // Отрисовка всех зон на экране
    void drawZones(const std::vector<std::vector<std::pair<int, int>>>& zones, int selectedZone = -1) {
        for (size_t i = 0; i < zones.size(); i++) {
            // Выбор цвета: красный для выбранной зоны, серый для остальных
            uint16_t color = (i + 1 == selectedZone) ? ST77XX_RED : ST77XX_GRAY;
            for (size_t j = 0; j < zones[i].size(); j++) {
                // Преобразование координат из миллиметров в пиксели
                int x = map(-zones[i][j].first, -6000, 6000, tft.width(), 0);
                int y = map(zones[i][j].second, 0, 6000, tft.height(), 0);
                tft.fillCircle(x, y, 5, color);  // Рисуем точку
                if (j > 0) {
                    // Рисуем линию между точками
                    int prevX = map(-zones[i][j - 1].first, -6000, 6000, tft.width(), 0);
                    int prevY = map(zones[i][j - 1].second, 0, 6000, tft.height(), 0);
                    tft.drawLine(prevX, prevY, x, y, color);
                }
            }
            // Замыкаем зону, соединяя первую и последнюю точки
            if (zones[i].size() > 1) {
                int firstX = map(-zones[i][0].first, -6000, 6000, tft.width(), 0);
                int firstY = map(zones[i][0].second, 0, 6000, tft.height(), 0);
                int lastX = map(-zones[i].back().first, -6000, 6000, tft.width(), 0);
                int lastY = map(zones[i].back().second, 0, 6000, tft.height(), 0);
                tft.drawLine(lastX, lastY, firstX, firstY, color);
            }
        }
    }

    // Отображение списка зон в меню справа
    void displayZonesInMenu(int zoneCount, int selectedZone, bool deleteMode) {
        tft.fillRect(tft.width() - 100, tft.height() - 40 - (zoneCount + 1) * 20, 100, (zoneCount + 1) * 20, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
        // Отображаем зоны в обратном порядке (сверху вниз)
        for (int i = zoneCount; i > 0; i--) {
            tft.setCursor(tft.width() - 100, tft.height() - 40 - (zoneCount - i + 1) * 20);
            if (selectedZone == i) {
                tft.setTextSize(2);  // Жирный шрифт для выбранной зоны
                if (deleteMode) tft.print("delete");
                else { tft.print("zone "); tft.print(i); }
            } else {
                tft.setTextSize(1);  // Обычный шрифт
                tft.print("zone "); tft.print(i);
            }
        }
        // Отображаем "new zone" внизу меню
        tft.fillRect(tft.width() - 100, tft.height() - 40, 100, 20, ST77XX_BLACK);
        tft.setCursor(tft.width() - 100, tft.height() - 40);
        if (selectedZone == -1) tft.setTextSize(2);  // Жирный шрифт, если выбран "new zone"
        else tft.setTextSize(1);
        tft.print("new zone");
    }

    // Очистка области меню
    void clearMenu(int zoneCount) {
        tft.fillRect(tft.width() - 100, tft.height() - 40 - (zoneCount + 1) * 20, 100, (zoneCount + 1) * 20 + 50, ST77XX_BLACK);
    }

    // Возвращаем объект дисплея для прямого доступа (если потребуется)
    Adafruit_ST7789& getTft() { return tft; }
};

// =============================================
// Класс Lidar: Работа с радарным датчиком LD2450
// =============================================
class Lidar {
private:
    LD2450 ld2450;  // Объект для работы с лидаром

public:
    // Инициализация последовательного порта для лидара
    void init() {
        Serial1.begin(256000, SERIAL_8N1, 16, 17);
        ld2450.begin(Serial1, true);
    }

    // Чтение количества обнаруженных целей
    int readTargets() { return ld2450.read(); }

    // Получение информации о цели по индексу
    LD2450::RadarTarget getTarget(int index) { return ld2450.getTarget(index); }
};

// =============================================
// Класс SDCard: Работа с SD-картой
// =============================================
class SDCard {
private:
    bool sdCardPresent = false;  // Флаг наличия SD-карты

public:
    // Инициализация SD-карты
    bool init() {
        SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
        sdCardPresent = SD.begin(SD_CS, SPI, 4000000);
        return sdCardPresent;
    }

    // Проверка наличия SD-карты
    bool isPresent() { return sdCardPresent; }

    // Сохранение зоны на SD-карту
    void saveZone(const std::vector<std::pair<int, int>>& zone) {
        if (!sdCardPresent) return;
        File file = SD.open("/src/zone.txt", FILE_APPEND);
        if (!file) return;
        for (size_t i = 0; i < zone.size(); i++) {
            if (i > 0) file.print(";");
            file.print(zone[i].first); file.print(","); file.print(zone[i].second);
        }
        file.println();
        file.close();
    }

    // Сохранение координат цели на SD-карту
    void saveCoordinates(int x, int y, const String& timeStr) {
        if (!sdCardPresent) return;
        File file = SD.open("/src/coordinates.txt", FILE_APPEND);
        if (!file) return;
        file.print(x); file.print(","); file.print(y); file.print(","); file.print(timeStr); file.println();
        file.close();
    }

    // Чтение зон с SD-карты
    void readZones(std::vector<std::vector<std::pair<int, int>>>& zones) {
        if (!sdCardPresent) return;
        File file = SD.open("/src/zone.txt");
        if (!file) return;
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.length() > 0) {
                std::vector<std::pair<int, int>> zone;
                int start = 0, end = line.indexOf(';');
                while (end != -1) {
                    String point = line.substring(start, end);
                    int comma = point.indexOf(',');
                    if (comma != -1) {
                        int x = point.substring(0, comma).toInt();
                        int y = point.substring(comma + 1).toInt();
                        zone.push_back({x, y});
                    }
                    start = end + 1;
                    end = line.indexOf(';', start);
                }
                String point = line.substring(start);
                int comma = point.indexOf(',');
                if (comma != -1) {
                    int x = point.substring(0, comma).toInt();
                    int y = point.substring(comma + 1).toInt();
                    zone.push_back({x, y});
                }
                zones.push_back(zone);
            }
        }
        file.close();
    }

    // Чтение данных Wi-Fi с SD-карты
    void readWiFiCredentials(String& ssid, String& password, int& utcOffset) {
        if (!sdCardPresent) return;
        File file = SD.open("/src/Wi-Fi.txt");
        if (!file) return;
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.startsWith("name:")) ssid = line.substring(5);
            else if (line.startsWith("password:")) password = line.substring(9);
            else if (line.startsWith("UTC:")) utcOffset = line.substring(4).toInt();
        }
        file.close();
    }
};

// =============================================
// Класс WiFiManager: Управление Wi-Fi и временем
// =============================================
class WiFiManager {
private:
    WiFiUDP ntpUDP;
    NTPClient timeClient{ntpUDP, "pool.ntp.org"};  // NTP-клиент для синхронизации времени

public:
    // Подключение к Wi-Fi
    bool connect(const String& ssid, const String& password, int utcOffset) {
        WiFi.begin(ssid.c_str(), password.c_str());
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) delay(500);
        if (WiFi.status() == WL_CONNECTED) {
            timeClient.begin();
            timeClient.setTimeOffset(3600 * utcOffset);
            return true;
        }
        return false;
    }

    // Проверка подключения к Wi-Fi
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }

    // Обновление времени
    void updateTime() { timeClient.update(); }

    // Получение текущего времени в формате строки
    String getCurrentTime() {
        time_t now = timeClient.getEpochTime();
        struct tm * timeinfo = localtime(&now);
        char timeStr[20];
        sprintf(timeStr, "%04d:%02d:%02d:%02d:%02d:%02d",
                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        return String(timeStr);
    }
};

// =============================================
// Класс Button: Обработка нажатий кнопки
// =============================================
class Button {
private:
    const int BOOT_BUTTON_PIN = 0;  // Пин кнопки
    volatile unsigned long buttonPressStartTime = 0;  // Время начала нажатия
    volatile bool buttonPressed = false;  // Флаг нажатия
    volatile bool buttonHandled = false;  // Флаг обработки нажатия
    volatile unsigned long lastPressTime = 0;  // Время последнего нажатия
    volatile bool flagRaised = false;  // Флаг короткого нажатия
    volatile unsigned long flagRaiseTime = 0;  // Время поднятия флага
    const unsigned long debounceDelay = 200;  // Задержка для устранения дребезга
    const unsigned long doublePressDelay = 500;  // Задержка для двойного нажатия
    const unsigned long longPressDelay = 2000;  // Задержка для длинного нажатия

public:
    // Инициализация кнопки и прерывания
    void init() {
        pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON_PIN), [this]() { this->handleInterrupt(); }, CHANGE);
    }

    // Обработчик прерывания от кнопки
    void handleInterrupt() {
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            buttonPressStartTime = millis();
            buttonPressed = true;
            buttonHandled = false;
        } else {
            if (buttonPressed && !buttonHandled && (millis() - lastPressTime > debounceDelay)) {
                if (millis() - lastPressTime < doublePressDelay) {
                    Serial.println("Double press");
                } else {
                    if (!flagRaised) {
                        flagRaised = true;
                        flagRaiseTime = millis();
                    }
                }
                buttonHandled = true;
                lastPressTime = millis();
            }
            buttonPressed = false;
        }
    }

    // Проверка на двойное нажатие
    bool isDoublePress() { return millis() - lastPressTime < doublePressDelay; }

    // Проверка на длинное нажатие
    bool isLongPress() { return buttonPressed && (millis() - buttonPressStartTime >= longPressDelay); }
};

// =============================================
// Класс ZoneManager: Управление зонами
// =============================================
class ZoneManager {
private:
    std::vector<std::vector<std::pair<int, int>>> zones;  // Список всех зон
    std::vector<int> redPointsX;  // Координаты X красных точек
    std::vector<int> redPointsY;  // Координаты Y красных точек
    int pointCount = 3;  // Количество точек для создания зоны
    bool zoneCreationMode = false;  // Флаг режима создания зоны
    bool zoneSaved = false;  // Флаг сохранения зоны

public:
    // Добавление точки в список красных точек
    void addPoint(int x, int y) {
        redPointsX.push_back(x);
        redPointsY.push_back(y);
    }

    // Сохранение зоны на SD-карту
    void saveZone(SDCard& sdCard) {
        std::vector<std::pair<int, int>> newZone;
        for (size_t i = 0; i < redPointsX.size(); i++) {
            // Преобразование координат из пикселей в миллиметры
            int x_mm = map(redPointsX[i], 0, 240, -6000, 6000);
            int y_mm = map(redPointsY[i], 0, 320, 6000, 0);
            newZone.push_back({x_mm, y_mm});
        }
        zones.push_back(newZone);
        sdCard.saveZone(newZone);
        zoneSaved = true;
    }

    // Удаление зоны по номеру
    void deleteZone(int zoneNumber, SDCard& sdCard) {
        if (zoneNumber < 1 || zoneNumber > zones.size()) return;
        zones.erase(zones.begin() + zoneNumber - 1);
        // Здесь можно добавить логику для обновления файла на SD-карте
    }

    // Отрисовка всех зон на экране
    void drawZones(Display& display, int selectedZone = -1) {
        display.drawZones(zones, selectedZone);
    }

    // Получение списка зон
    const std::vector<std::vector<std::pair<int, int>>>& getZones() { return zones; }
};

// =============================================
// Класс RadarSystem: Главный класс системы
// =============================================
class RadarSystem {
private:
    Display display;  // Дисплей
    Lidar lidar;  // Лидар
    SDCard sdCard;  // SD-карта
    WiFiManager wifiManager;  // Wi-Fi и время
    Button button;  // Кнопка
    ZoneManager zoneManager;  // Управление зонами
    bool setupMode = false;  // Флаг режима настройки
    bool deleteMode = false;  // Флаг режима удаления
    int selectedZone = -1;  // Выбранная зона
    int pointCount = 3;  // Количество точек для создания зоны
    bool showPointText = true;  // Флаг отображения текста "point"
    bool showCheckmark = false;  // Флаг отображения галочки
    unsigned long checkmarkStartTime = 0;  // Время начала отображения галочки
    int prevX[3] = {-1, -1, -1}, prevY[3] = {-1, -1, -1};  // Предыдущие координаты целей
    int prevTargetX = 0, prevTargetY = 0;  // Предыдущие координаты первой цели

public:
    // Инициализация всех компонентов системы
    void init() {
        display.init();
        lidar.init();
        sdCard.init();
        button.init();
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);
        String ssid, password;
        int utcOffset;
        sdCard.readWiFiCredentials(ssid, password, utcOffset);
        wifiManager.connect(ssid, password, utcOffset);
        sdCard.readZones(zoneManager.getZones());
    }

    // Основной цикл программы
    void loop() {
        // Обновление времени, если подключены к Wi-Fi
        if (wifiManager.isConnected()) wifiManager.updateTime();

        // Обработка длинного нажатия кнопки (переключение режима настройки)
        if (button.isLongPress()) {
            setupMode = !setupMode;
            if (!setupMode) {
                display.clearMenu(zoneManager.getZones().size());
                selectedZone = -1;
                deleteMode = false;
            }
        }

        // Чтение данных с лидара
        int sensorTargets = lidar.readTargets();
        if (sensorTargets > 0) {
            for (int i = 0; i < (lidar.getTarget(0).valid ? 1 : 0); i++) {
                LD2450::RadarTarget target = lidar.getTarget(i);
                // Преобразование координат цели в пиксели экрана
                int x = map(target.x, -6000, 6000, display.getTft().width(), 0);
                int y = map(target.y, 0, 6000, display.getTft().height(), 0);
                // Отрисовка цели на экране
                display.getTft().fillCircle(x, y, 5, ST77XX_WHITE);
                if (i == 0) {
                    // Отображение координат первой цели внизу экрана
                    display.getTft().setCursor(0, display.getTft().height() - 20);
                    display.getTft().setTextColor(ST77XX_WHITE);
                    display.getTft().print("X: "); display.getTft().print(target.x);
                    display.getTft().print("mm, Y: "); display.getTft().print(target.y); display.getTft().print("mm");
                    // Сохранение координат цели на SD-карту
                    sdCard.saveCoordinates(target.x, target.y, wifiManager.getCurrentTime());
                }
            }
        }

        // Отрисовка интерфейса в зависимости от режима
        if (setupMode) {
            display.displayZonesInMenu(zoneManager.getZones().size(), selectedZone, deleteMode);
            zoneManager.drawZones(display, selectedZone);
        } else {
            display.clearMenu(zoneManager.getZones().size());
            zoneManager.drawZones(display);
        }
    }
};

// --- Главные функции Arduino ---
RadarSystem radarSystem;  // Создание объекта главной системы

void setup() {
    Serial.begin(115200);  // Инициализация последовательного порта
    radarSystem.init();  // Инициализация системы
}

void loop() {
    radarSystem.loop();  // Основной цикл программы
}

