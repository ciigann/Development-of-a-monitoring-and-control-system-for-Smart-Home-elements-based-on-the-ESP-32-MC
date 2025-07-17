// Include the HardwareSerial library for UART communication
#include <HardwareSerial.h>

// Define RX and TX pins for the LD2420 sensor
#define RX_PIN 23  // ESP32 pin connected to LD2420 TX (OT1)
#define TX_PIN 22  // ESP32 pin connected to LD2420 RX

// Create a HardwareSerial object for UART2
HardwareSerial ld2420Serial(2); // UART2 on ESP32

void setup() {
  // Initialize the serial communication with the LD2420
  ld2420Serial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Default baud rate for LD2420
  Serial.begin(9600);                                     // Serial monitor for debugging

  Serial.println("LD2420 MMWave Sensor Initialized");
}

void loop() {
  // Check if data is available from the LD2420
  if (ld2420Serial.available()) {
    // Read and print the data from the sensor
    String sensorData = ld2420Serial.readStringUntil('\n');
    Serial.println("Sensor Data: " + sensorData);
  }

  delay(100); // Small delay to avoid overwhelming the serial buffer
}

