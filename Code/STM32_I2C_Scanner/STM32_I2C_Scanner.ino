#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(9600);

  // Setup the LED pin
  pinMode(PC13, OUTPUT);

  // Initialize I2C2 on PB11 (SDA) and PB10 (SCL)
  Wire.setSDA(PB11);
  Wire.setSCL(PB10);
  Wire.begin();

  // Scan I2C bus
  Serial.println("Scanning I2C bus on I2C2...");
  scanI2C();
}

void loop() {
  // Blink the LED
  digitalWrite(PC13, HIGH);
  // Continuously scan the I2C bus
  scanI2C();
  digitalWrite(PC13, LOW);
  delay(5000);
}

void scanI2C() {
  byte error, address;
  int nDevices = 0;

  Serial.println("I2C Devices:");
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found device at 0x");
      Serial.println(address, HEX);
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("Scan complete\n");
  }
}
