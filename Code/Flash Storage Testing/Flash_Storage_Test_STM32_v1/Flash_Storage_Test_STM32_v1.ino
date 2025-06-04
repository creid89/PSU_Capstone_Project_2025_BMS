#include <Wire.h>
#include <EEPROM.h>

#define EEPROM_ADDR_1  0
#define EEPROM_ADDR_2  (EEPROM_ADDR_1 + sizeof(float))
#define EEPROM_ADDR_3  (EEPROM_ADDR_2 + sizeof(float))
#define EEPROM_ADDR_4  (EEPROM_ADDR_3 + sizeof(float))
#define EEPROM_ADDR_5  (EEPROM_ADDR_4 + sizeof(float))
#define EEPROM_ADDR_6  (EEPROM_ADDR_5 + sizeof(float))
#define EEPROM_ADDR_7  (EEPROM_ADDR_6 + sizeof(float))
#define EEPROM_ADDR_8  (EEPROM_ADDR_7 + sizeof(float))
#define EEPROM_TOTAL_LENGTH (8 * sizeof(float))  // 32 bytes

float storedValue1;
float storedValue2;

// Check if EEPROM is initialized
bool isEEPROMInitialized(int startAddr, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (EEPROM.read(startAddr + i) != 0xFF) {
      return true;
    }
  }
  return false;
}

// Wipe EEPROM by filling with 0xFF
void clearEEPROM() {
  for (int i = 0; i < EEPROM_TOTAL_LENGTH; i++) {
    EEPROM.write(i, 0xFF);
  }
  Serial.println("EEPROM wiped to 0xFF. Reboot to confirm.");
}

// Dump EEPROM byte-by-byte for inspection
void dumpEEPROM(int startAddr, size_t length) {
  Serial.println("EEPROM RAW DUMP:");
  for (size_t i = 0; i < length; i++) {
    if (i % 8 == 0) Serial.print("0x" + String(startAddr + i, HEX) + ": ");
    Serial.print(EEPROM.read(startAddr + i), HEX);
    Serial.print(" ");
    if (i % 8 == 7) Serial.println();
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== EEPROM Test Utility ===");

  if (isEEPROMInitialized(EEPROM_ADDR_1, EEPROM_TOTAL_LENGTH)) {
    EEPROM.get(EEPROM_ADDR_1, storedValue1);
    EEPROM.get(EEPROM_ADDR_2, storedValue2);

    Serial.println("EEPROM has stored values:");
    Serial.print("storedValue1: "); Serial.println(storedValue1, 4);
    Serial.print("storedValue2: "); Serial.println(storedValue2, 4);
  } else {
    Serial.println("EEPROM is blank or uninitialized.");
  }

  dumpEEPROM(EEPROM_ADDR_1, EEPROM_TOTAL_LENGTH);

  Serial.println("\nType two float values separated by a space (e.g., '12.34 56.78') to store.");
  Serial.println("Or type 'CLEAR' to erase EEPROM.");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("CLEAR")) {
      clearEEPROM();
      return;
    }

    int spaceIndex = input.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println("Invalid input. Please enter two float values or 'CLEAR'.");
      return;
    }

    String val1Str = input.substring(0, spaceIndex);
    String val2Str = input.substring(spaceIndex + 1);

    float val1 = val1Str.toFloat();
    float val2 = val2Str.toFloat();

    EEPROM.put(EEPROM_ADDR_1, val1);
    EEPROM.put(EEPROM_ADDR_2, val2);

    Serial.println("Values written to EEPROM.");
    Serial.println("Verifying...");

    EEPROM.get(EEPROM_ADDR_1, storedValue1);
    EEPROM.get(EEPROM_ADDR_2, storedValue2);

    Serial.print("storedValue1 (verified): "); Serial.println(storedValue1, 4);
    Serial.print("storedValue2 (verified): "); Serial.println(storedValue2, 4);

    dumpEEPROM(EEPROM_ADDR_1, EEPROM_TOTAL_LENGTH);
    Serial.println("Done. You may enter new values or 'CLEAR'.");
  }
}
