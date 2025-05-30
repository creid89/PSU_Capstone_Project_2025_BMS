#include <EEPROM.h>

#define EEPROM_ADDR 0  // Start address in emulated EEPROM

int storedValue;

void setup() {
  Serial.begin(115200);
  delay(500);  // Wait for Serial Monitor

  // Read stored value from flash
  EEPROM.get(EEPROM_ADDR, storedValue);
  Serial.print("Value read from flash: ");
  Serial.println(storedValue);

  Serial.println("Enter a new integer value to store (or leave blank to keep current):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      int newValue = input.toInt();
      Serial.print("Writing new value to flash: ");
      Serial.println(newValue);

      // Store the new value in flash
      EEPROM.put(EEPROM_ADDR, newValue);
      // No EEPROM.commit() needed on STM32!

      Serial.println("Value saved! Reboot or power cycle to test reload.");
    } else {
      Serial.println("No input provided. Keeping current value.");
    }

    Serial.println("Enter another value:");
  }
}
