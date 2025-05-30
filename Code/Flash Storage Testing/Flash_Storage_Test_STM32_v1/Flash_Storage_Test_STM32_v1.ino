#include <EEPROM.h>

#define EEPROM_ADDR_1 0                  // Start address for first float
#define EEPROM_ADDR_2 (EEPROM_ADDR_1 + sizeof(float))  // Next address for second float

float storedValue1;
float storedValue2;

void setup() {
  Serial.begin(115200);
  delay(500);  // Wait for Serial Monitor

  // Read stored values from flash
  EEPROM.get(EEPROM_ADDR_1, storedValue1);
  EEPROM.get(EEPROM_ADDR_2, storedValue2);

  Serial.print("Value read storedValue1 from flash: ");
  Serial.println(storedValue1);
  Serial.print("Value read storedValue2 from flash: ");
  Serial.println(storedValue2);

  Serial.println("\nEnter two float values separated by a space (e.g., '12.34 56.78'):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      // Split the input by space
      int spaceIndex = input.indexOf(' ');
      if (spaceIndex == -1) {
        Serial.println("Please provide two float values separated by a space.");
        return;
      }

      String val1Str = input.substring(0, spaceIndex);
      String val2Str = input.substring(spaceIndex + 1);

      float newValue1 = val1Str.toFloat();
      float newValue2 = val2Str.toFloat();

      Serial.print("Writing storedValue1 to flash: ");
      Serial.println(newValue1);
      EEPROM.put(EEPROM_ADDR_1, newValue1);

      Serial.print("Writing storedValue2 to flash: ");
      Serial.println(newValue2);
      EEPROM.put(EEPROM_ADDR_2, newValue2);

      Serial.println("Values saved! Reboot or power cycle to test reload.");
    } else {
      Serial.println("No input provided. Keeping current values.");
    }

    Serial.println("\nEnter two float values separated by a space:");
  }
}
