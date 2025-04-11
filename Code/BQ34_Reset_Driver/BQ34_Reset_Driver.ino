// BQ34Z100 Reset & Status Tool - Team 6 BMS
// Standalone utility to reset fuel gauge and read FW/HW version + control flags
#include <Wire.h>

#define BQ34Z100_ADDR 0x55
#define CMD_CONTROL   0x00

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("--- BQ34Z100 Reset & Status Tool ---");

  delay(1000);
  Serial.println("Attempting to unseal device...");
  unsealBQ34Z100();
  delay(500);

  Serial.println("Preparing to reset BQ34Z100...");
  delay(1000);
  Serial.println("WARNING: Reset will be issued in 5 seconds...");
  delay(5000);

  resetBQ34Z100();

  Serial.println("Reset command sent. Waiting for device to stabilize...");
  delay(3000);

  uint16_t fw = getFirmwareVersion();
  uint16_t hw = getHardwareVersion();

  if (fw == 0xFFFF || hw == 0xFFFF) {
    Serial.println("Warning: Unable to read version data. Gauge may still be rebooting or sealed.");
  }

  Serial.print("Firmware Version: 0x"); Serial.println(fw, HEX);
  Serial.print("Hardware Version: 0x"); Serial.println(hw, HEX);

  Serial.println("\nReading CONTROL_STATUS flags...");
  uint16_t ctrlStatus = getControlStatus();
  printControlStatusFlags(ctrlStatus);
}

void loop() {
  // nothing
}

void resetBQ34Z100() {
  Serial.println("Sending RESET command (0x0041)...");
  sendControlCommand(0x0041);
}

void unsealBQ34Z100() {
  sendControlCommand(0x8000);
  delay(10);
  sendControlCommand(0x8000);
  delay(100);
  Serial.println("Unseal command sent.");
}

uint16_t getFirmwareVersion() {
  sendControlCommand(0x0002);
  delay(100);
  return readWord(0x00);
}

uint16_t getHardwareVersion() {
  sendControlCommand(0x0003);
  delay(100);
  return readWord(0x00);
}

uint16_t getControlStatus() {
  sendControlCommand(0x0000); // CONTROL_STATUS
  delay(10);
  return readWord(0x00);
}

void printControlStatusFlags(uint16_t status) {
  Serial.println("\nCONTROL_STATUS Flags:");
  Serial.println("Bit  | Flag       | State");
  Serial.println("---------------------------");

  const char* flags[16] = {
    "QEN", "VOK", "RUP_DIS", "LDMD", "SLEEP", "FULLSLEEP", "-", "-",
    "-", "CSV", "BCA", "CCA", "CALMODE", "SS", "FAS", "-"
  };

  for (int i = 15; i >= 0; --i) {
    if (flags[i][0] != '-') {
      Serial.print(" "); Serial.print(i);
      Serial.print("   | ");
      Serial.print(flags[i]);
      Serial.print("   | ");
      Serial.println((status & (1 << i)) ? "SET" : "CLEAR");
    }
  }
}

void sendControlCommand(uint16_t subcmd) {
  Wire.beginTransmission(BQ34Z100_ADDR);
  Wire.write(CMD_CONTROL);
  Wire.write((uint8_t)(subcmd & 0xFF));
  Wire.write((uint8_t)(subcmd >> 8));
  Wire.endTransmission();
}

uint16_t readWord(uint8_t reg) {
  Wire.beginTransmission(BQ34Z100_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;
  Wire.requestFrom(BQ34Z100_ADDR, (uint8_t)2);
  if (Wire.available() < 2) return 0xFFFF;
  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();
  return (msb << 8) | lsb;
}
