#include "Arduino.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

//#include <HardwareSerial.h>
#define TXD_PIN 0  // RP2040 TX (GP0)
#define RXD_PIN 1  // RP2040 RX (GP1)

String inputString = "";      // Stores incoming serial data
bool stringComplete = false;  // Indicates when a full line has been received

// Check if the input string contains only valid HEX characters (0-9, A-F, a-f)
bool isValidHex(String s) {
  s.trim();  // Remove leading/trailing whitespace
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (!((c >= '0' && c <= '9') ||
          (c >= 'A' && c <= 'F') ||
          (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(9600); // Initialize USB Serial for menu interface
  while (!Serial) ;   // Wait for Serial monitor to open

  // Initialize UART0 (default UART for serial communication) on specified pins with 9600 baud
  Serial2.begin(9600); // Initialize hardware/control bus Serial for menu interface

  Serial.println("UART HEX Data Sender Menu");
  Serial.println("Enter HEX values (even number of hex digits) and press Enter:");
}

void loop() {
  if (stringComplete) {
    inputString.trim();
    // Validate that the input contains only HEX digits
    if (!isValidHex(inputString)) {
      Serial.println("Error: Invalid HEX input! Please use only 0-9, A-F.");
    } 
    // Ensure that an even number of hex digits is provided
    else if (inputString.length() % 2 != 0) {
      Serial.println("Error: Enter an even number of HEX digits.");
    } 
    else {
      int numBytes = inputString.length() / 2;
      uint8_t byteArray[numBytes];
      // Convert each pair of hex characters into a byte
      for (int i = 0; i < numBytes; i++) {
        String byteStr = inputString.substring(i * 2, i * 2 + 2);
        byteArray[i] = (uint8_t) strtol(byteStr.c_str(), NULL, 16);
      }
      // Send the byte array over UART1
      Serial1.write(byteArray, numBytes);
      
      // Print out the bytes in HEX format for debugging
      Serial.print("Sent HEX bytes: ");
      for (int i = 0; i < numBytes; i++) {
        Serial.print("0x");
        if (byteArray[i] < 0x10)
          Serial.print("0");
        Serial.print(byteArray[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    
    // Reset input for the next entry
    inputString = "";
    stringComplete = false;
    Serial.println("\nEnter HEX values (even number of hex digits) and press Enter:");
  }
}

// This function is automatically called when new data arrives on Serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    // Newline indicates the end of the input line
    if (inChar == '\n') {
      stringComplete = true;
    }
    // Ignore carriage returns
    else if (inChar != '\r') {  
      inputString += inChar;
    }
  }
}
