#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Определение пинов для подключения SD-карты
#define SD_CS 33
#define SD_SCLK 25
#define SD_MOSI 26
#define SD_MISO 27

void setup() {
  Serial.begin(115200);

  // Инициализация SPI для SD-карты
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // Удаление второй строки из файла test.txt
  removeLine(SD, "/test.txt", 3); // Удаляем вторую строку
}

void loop() {
  // Ничего не делаем в основном цикле
}

void removeLine(fs::FS &fs, const char *path, int lineToRemove) {
  Serial.printf("Removing line %d from file: %s\n", lineToRemove, path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  String fileContent = "";
  int lineCount = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    lineCount++;
    if (lineCount != lineToRemove) { // Пропускаем указанную строку
      fileContent += line + "\n";
    }
  }
  file.close();

  // Перезаписываем файл без указанной строки
  File fileToWrite = fs.open(path, FILE_WRITE);
  if (!fileToWrite) {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (fileToWrite.print(fileContent)) {
    Serial.println("Line removed successfully");
  } else {
    Serial.println("Write failed");
  }
  fileToWrite.close();
}
