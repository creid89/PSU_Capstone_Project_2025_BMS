#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX


void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }
  pinMode(PC13, OUTPUT);
}

void loop() {
  Serial.println("CONNECTED");
  digitalWrite(PC13, HIGH);
  delay(1000);
  digitalWrite(PC13, LOW);
  delay(1000);
}