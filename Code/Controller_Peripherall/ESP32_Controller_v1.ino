#include <Wire.h>

#define SLAVE_ADDRESS 0x58
#define SDA_PIN 23
#define SCL_PIN 22

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("ESP32 I2C Master Ready");
  Serial.println("Enter HEX command (01-08) or 'w10=3.14' to write float");
}

void loop() {
  if (stringComplete) {
    inputString.trim();
    if (inputString.length() == 0) {
      Serial.println("Empty input.");
    } else if (inputString.startsWith("w")) {
      // Write float format: w10=3.14
      int eqIndex = inputString.indexOf('=');
      if (eqIndex > 1) {
        String cmdStr = inputString.substring(1, eqIndex);
        String valStr = inputString.substring(eqIndex + 1);
        uint8_t cmd = (uint8_t)strtol(cmdStr.c_str(), NULL, 16);
        float val = valStr.toFloat();
        sendFloatCommand(cmd, val);
      }
    } else {
      // Send read command
      uint8_t command = (uint8_t) strtol(inputString.c_str(), NULL, 16);
      requestFloat(command);
    }

    inputString = "";
    stringComplete = false;
    Serial.println("\nEnter another command:");
  }
}

void sendFloatCommand(uint8_t command, float value) {
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(command);

  uint8_t buffer[4];
  memcpy(buffer, &value, sizeof(float));
  Wire.write(buffer, 4);

  uint8_t error = Wire.endTransmission();
  if (error == 0) {
    Serial.print("Sent float ");
    Serial.print(value, 5);
    Serial.print(" to command 0x");
    Serial.println(command, HEX);
  } else {
    Serial.print("Error sending float. Code: ");
    Serial.println(error);
  }
}

void requestFloat(uint8_t command) {
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(command);
  uint8_t error = Wire.endTransmission();
  if (error != 0) {
    Serial.print("Command send error: ");
    Serial.println(error);
    return;
  }

  Wire.requestFrom(SLAVE_ADDRESS, (uint8_t)4);
  if (Wire.available() < 4) {
    Serial.println("Error: did not receive 4 bytes");
    return;
  }

  uint8_t buf[4];
  for (int i = 0; i < 4; i++) buf[i] = Wire.read();
  float receivedVal;
  memcpy(&receivedVal, buf, 4);

  Serial.print("Received float: ");
  Serial.println(receivedVal, 5);
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else if (inChar != '\r') {
      inputString += inChar;
    }
  }
}
