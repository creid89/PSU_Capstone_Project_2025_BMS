#include <EEPROM.h>

#define EEPROM_ADDR_1 0                                      // Start address for first float
#define EEPROM_ADDR_2 (EEPROM_ADDR_1 + sizeof(float))        // Next address for second float
#define EEPROM_TOTAL_LENGTH (2 * sizeof(float))              // Total length for 2 floats

float storedValue1;
float storedValue2;

bool isEEPROMInitialized(int startAddr, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (EEPROM.read(startAddr + i) != 0xFF) {
      Serial.print("EEPROM has data");
      return true;  // Found non-blank byte
      
    }
  }
  Serial.print("EEPROM has NO data");
  return false;  // All bytes are 0xFF → uninitialized
  
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for Serial Monitor

  if (isEEPROMInitialized(EEPROM_ADDR_1, EEPROM_TOTAL_LENGTH)) {
    // Read stored values from flash
    EEPROM.get(EEPROM_ADDR_1, storedValue1);
    EEPROM.get(EEPROM_ADDR_2, storedValue2);

    Serial.print("Value read storedValue1 from flash: ");
    Serial.println(storedValue1);
    Serial.print("Value read storedValue2 from flash: ");
    Serial.println(storedValue2);
  } else {
    Serial.println("EEPROM is uninitialized or blank.");
    Serial.println("Please enter two float values to initialize it.");
  }

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
