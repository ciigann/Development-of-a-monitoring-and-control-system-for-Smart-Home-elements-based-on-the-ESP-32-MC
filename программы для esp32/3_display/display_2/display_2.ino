#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>             // Arduino SPI library

// Define the pins for the display
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   15
#define TFT_RST  4

// Create an instance of the display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void setup() {
  // Initialize the display
  tft.init(240, 320); // Initialize the display with the correct resolution
  tft.setRotation(1);  // Set the rotation (adjust as needed)
  tft.fillScreen(ST77XX_BLACK); // Clear the screen with black color

  // Display some text
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 10);
  tft.println("Hello, ST7789V2!");
}

void loop() {
  // Nothing to do here
}
