//BMS Project BQ34 Fuel Guage driver - Daniel Anishchenko - Team 6 BMS
//Utilizing Wire for Comms (I2C)
#include <Wire.h>

//Device
#define BQ34Z100_ADDR 0x55  // 7-bit I2C address

// SMBus Commands
#define CMD_CONTROL               0x00
#define CMD_TEMPERATURE           0x06
#define CMD_VOLTAGE               0x08
#define CMD_FLAGS                 0x0D
#define CMD_REMAINING_CAPACITY   0x2C
#define CMD_FULL_CHARGE_CAPACITY 0x2D
#define CMD_STATE_OF_CHARGE      0x2F
#define CMD_CURRENT               0x10
#define CMD_AVERAGE_CURRENT       0x14
#define CMD_CURR_SCALE            0x21  // Current Scale is at command 0x1E according to TRM
#define CMD_VOLT_SCALE            0x20  // Voltage Scale is at command 0x1F according to TRM

float currScale = 1.0;
float voltScale = 1.0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(1000);
  Serial.println("Initializing BQ34Z100...");

  delay(500);

  // Read initial flags to confirm device is responsive
  uint16_t flags = readWord(CMD_FLAGS);
  Serial.print("Flags: 0x"); Serial.println(flags, HEX);

  if (flags == 0xFFFF) {
    Serial.println("Error: No response from BQ34Z100. Check wiring.");
  } else {
    Serial.println("BQ34Z100 communication OK.");
  }

  currScale = readCurrScale();
  Serial.print("CurrScale value: "); Serial.println(currScale, 6);

  voltScale = readVoltScale();
  Serial.print("VoltScale value: "); Serial.println(voltScale, 6);
}

void loop() {
  uint16_t voltageRaw = readWord(CMD_VOLTAGE);
  uint16_t remainingCapacity = readWord(CMD_REMAINING_CAPACITY);
  uint16_t fullChargeCapacity = readWord(CMD_FULL_CHARGE_CAPACITY);
  uint16_t soc = readWord(CMD_STATE_OF_CHARGE);
  uint16_t temp = readWord(CMD_TEMPERATURE);
  int16_t avgCurrentRaw = (int16_t)readWord(CMD_AVERAGE_CURRENT);
  float realCurrent = avgCurrentRaw * currScale;
  float realVoltage = voltageRaw/voltScale ;

  Serial.println("-------------------------");
  Serial.print("Voltage: "); Serial.print(realVoltage); Serial.println(" V");
  Serial.print("Remaining Capacity: "); Serial.print(remainingCapacity); Serial.println(" mAh");
  Serial.print("Full Charge Capacity: "); Serial.print(fullChargeCapacity); Serial.println(" mAh");
  Serial.print("State of Charge: "); Serial.print(soc); Serial.println(" %");
  Serial.print("Temperature: "); Serial.print((temp/10)-273.15, 1); Serial.println(" C");
  Serial.print("Average Current: "); Serial.print(realCurrent); Serial.println(" mA");

  delay(3000);
}

// Read a 2-byte word from a register
uint16_t readWord(uint8_t reg) {
  Wire.beginTransmission(BQ34Z100_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0xFFFF; // error
  }

  Wire.requestFrom(BQ34Z100_ADDR, (uint8_t)2);
  if (Wire.available() < 2) {
    return 0xFFFF; // error
  }

  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();
  return (msb << 8) | lsb;
}

// Read CurrScale using standard command 0x1E
float readCurrScale() {
  uint16_t raw = readWord(CMD_CURR_SCALE);
  if (raw == 0xFFFF) {
    return 1.0; // fallback default
  }
  return (float)raw;
}

// Read VoltScale using standard command 0x1F
float readVoltScale() {
  uint16_t raw = readWord(CMD_VOLT_SCALE);
  if (raw == 0xFFFF) {
    return 1.0; // fallback default
  }
  return (float)raw;
}
