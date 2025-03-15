#include <Wire.h>

#define SLAVE_ADDRESS 0x60  // STM32 I2C slave address
#define SDA_PIN 23          // Adjust if needed
#define SCL_PIN 22          // Adjust if needed

String inputString = "";      // Holds user input from Serial
bool stringComplete = false;  // Flag for when input is complete

void setup() {
  Serial.begin(115200);
  delay(2000);  // Allow time for the Serial Monitor to start

  // Initialize I2C as Master with specified SDA and SCL pins
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("ESP32 I2C Master Initialized.");
  Serial.println("Enter HEX command to send (e.g., 01, A3, etc):");
}

void loop() {
  // When a complete line is received, process it
  if (stringComplete) {
    inputString.trim();
    if (inputString.length() == 0) {
      Serial.println("Empty input. Please enter a valid HEX command:");
    } else {
      // Convert the input string (assumed to be in hexadecimal) into a byte
      uint8_t command = (uint8_t) strtol(inputString.c_str(), NULL, 16);
      
      Serial.print("Sending command: 0x");
      Serial.println(command, HEX);
      
      // Begin I2C transmission to the slave and send the command
      Wire.beginTransmission(SLAVE_ADDRESS);
      Wire.write(command);
      uint8_t error = Wire.endTransmission();
      
      if (error == 0) {
        Serial.println("Command sent successfully.");
      } else {
        Serial.print("Error sending command. Code: ");
        Serial.println(error);
      }
      
      // Now request 2 bytes from the slave as a response
      Serial.println("Requesting 2 bytes from slave...");
      uint8_t numBytes = 2;
      Wire.requestFrom(SLAVE_ADDRESS, numBytes);
      
      if(Wire.available() < numBytes) {
        Serial.println("Error: Did not receive expected number of bytes");
      }
      else {
        uint8_t data1 = Wire.read();
        //uint8_t data2 = Wire.read();
        Serial.print("Received bytes: 0x");
        if(data1 < 0x10) Serial.print("0");
        Serial.print(data1, HEX);
        /*Serial.print(" 0x");
        if(data2 < 0x10) Serial.print("0");
        Serial.println(data2, HEX);*/
      }
    }
    // Reset inputString and prompt the user again
    inputString = "";
    stringComplete = false;
    Serial.println("\nEnter HEX command to send:");
  }
}

// This function is called automatically whenever new data arrives on Serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    }
    // Ignore carriage returns
    else if (inChar != '\r') {
      inputString += inChar;
    }
  }
}
