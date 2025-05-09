#include <Wire.h>

#define BQ25730_ADDR 0x6B  // 7-bit I2C address

// Register addresses
#define ChargeOption0   0x00
#define ChargeCurrent   0x02
#define ChargeVoltage   0x04
#define OTGVoltage      0x06
#define OTGCurrent      0x08
#define InputVoltage    0x0A
#define VSYS_MIN        0x0C
#define IIN_HOST        0x0E
#define ChargerStatus   0x20
#define ProchotStatus   0x22
#define IIN_DPM         0x24
#define AADCVBUS_PSYS   0x26
#define ADCIBAT         0x28
#define ADCIINCMPIN     0x2A
#define ADCVSYSVBAT     0x2C
#define MANUFACTURER_ID 0x2E
#define DEVICE_ID       0x2F
#define ChargeOption1   0x30
#define ChargeOption2   0x32
#define ChargeOption3   0x34
#define ProchotOption0  0x36
#define ProchotOption1  0x38
#define ADCOption       0x3A
#define ChargeOption4   0x3C
#define Vmin_Active_Protection 0x3E

// Configuration parameters
float ChargeVoltageValue = 17.000;         // Charging target voltage
float VsysMinValue = 13.3;                 // Minimum system voltage
float inputVoltageValue = 12.0;            // Input voltage limit (used for VINDPM)
float chargeCurrentValue = 1.00;           // Battery charging current (in amps)
float inputCurrentLimitValue = 2100.00;    // Input current limit (in milliamps) - based on supply/adapter limit

unsigned long lastCheck1 = 0;
const unsigned long I2CcheckInterval = 5000;

unsigned long lastCheck2 = 0;
const unsigned long registerUpdateInterval = 2500;

void setup() {
  Serial.begin(9600);
  delay(5000);
  Wire.begin();

  Serial.println("BQ25730 Control Initialization");

  if (!checkBQConnection()) {
    Serial.println("BQ25730 not responding on I2C. Check wiring or power.");
    while (1); // Halt system
  } else {
    Serial.println("BQ25730 connection verified.");
  }

  // Configure initial charger settings
  setChargeVoltage(ChargeVoltageValue);
  setInputVoltage(inputVoltageValue);
  setChargeCurrent(chargeCurrentValue);
  setInputCurrentLimit(inputCurrentLimitValue);
  set_VSYS_MIN(VsysMinValue);
  enableADC();
  enableCharging();
}

void loop() {
  // Periodically check I2C communication
  if (millis() - lastCheck1 > I2CcheckInterval) {
    lastCheck1 = millis();
    if (!checkI2C()) {
      Serial.println("I2C comms lost. Attempting to reinitialize...");
      Wire.end();
      delay(100);
      Wire.begin();
      delay(100);
    }
  }

  // Periodically re-write voltage/current registers to maintain charging
  if (millis() - lastCheck2 > registerUpdateInterval) {
    lastCheck2 = millis();
    setChargeVoltage(ChargeVoltageValue);
    setChargeCurrent(chargeCurrentValue);
    dumpAllRegisters();
  }
  
}

// -----------------------------
// Register Read/Write Utilities
// -----------------------------

void writeBQ25730(uint8_t regAddr, uint16_t data) {
  Wire.beginTransmission(BQ25730_ADDR);
  Wire.write(regAddr);
  Wire.write(data & 0xFF);          // Low byte
  Wire.write((data >> 8) & 0xFF);   // High byte
  Wire.endTransmission();
  delay(5);
}

uint16_t readBQ25730(uint8_t regAddr) {
  Wire.beginTransmission(BQ25730_ADDR);
  Wire.write(regAddr);
  Wire.endTransmission(false);
  Wire.requestFrom(BQ25730_ADDR, 2);
  uint16_t data = 0;
  if (Wire.available() == 2) {
    data = Wire.read();
    data |= Wire.read() << 8;
  }
  return data;
}

bool checkI2C() {
  uint16_t id = readBQ25730(DEVICE_ID);
  return (id != 0x0000 && id != 0xFFFF);
}

bool checkBQConnection() {
  Wire.beginTransmission(BQ25730_ADDR);
  return (Wire.endTransmission() == 0);
}

// -----------------------------
// Charger Configuration
// -----------------------------

void setChargeVoltage(float voltage_V) {
  if (voltage_V < 1.024 || voltage_V > 23.000) {
    Serial.println("Charge voltage out of range (1.024V – 23.000V).");
    return;
  }

  uint16_t stepCount = static_cast<uint16_t>(voltage_V / 0.008 + 0.5);
  uint16_t encoded = (stepCount & 0x1FFF) << 3;

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to ChargeVoltage ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(ChargeVoltage, encoded);
}

void setChargeCurrent(float current_A) {
  if (current_A < 0.0 || current_A > 16.256) {
    Serial.println("Charge current out of range (0 – 16.256 A).");
    return;
  }

  uint16_t stepCount = static_cast<uint16_t>(current_A / 0.128 + 0.5);
  uint16_t encoded = (stepCount << 6) & 0x1FC0;

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to ChargeCurrent Register ("); Serial.print(current_A); Serial.println(" A)");
  writeBQ25730(ChargeCurrent, encoded);
}

void setInputCurrentLimit(float current_mA) {
  if (current_mA < 50.0f) current_mA = 50.0f;
  if (current_mA > 6350.0f) current_mA = 6350.0f;

  uint16_t code = static_cast<uint16_t>(current_mA / 50.0f);
  uint16_t regValue = (code << 8) & 0x7F00;

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(regValue, HEX);
  Serial.print(" to IIN_HOST Register ("); Serial.print(current_mA); Serial.println(" mA)");
  writeBQ25730(IIN_HOST, regValue);
}

void setInputVoltage(float voltage_V) {
  if (voltage_V < 3.2 || voltage_V > 23.0) {
    Serial.println("Input voltage out of range (3.2V – 23.0V).");
    return;
  }

  uint16_t stepCount = static_cast<uint16_t>((voltage_V - 3.2) / 0.064 + 0.5);
  uint16_t encoded = (stepCount << 6) & 0x3FC0;

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to InputVoltage Register ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(InputVoltage, encoded);
}

void set_VSYS_MIN(float voltage_V) {
  if (voltage_V < 1.0 || voltage_V > 23.0) {
    Serial.println("VSYS_MIN voltage out of range (1.0V – 23.0V).");
    return;
  }

  uint16_t encoded = static_cast<uint16_t>(voltage_V * 10) << 8;

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to VSYS_MIN ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(VSYS_MIN, encoded);
}

void enableCharging() {
  // 0xE70E: disables watchdog and enables charging permanently
  writeBQ25730(ChargeOption0, 0xE70E);
}

void disableCharging() {
  uint16_t option0 = readBQ25730(ChargeOption0);
  //option0 &= ~(1 << 0); // Clear CHRG_INHIBIT bit
  writeBQ25730(ChargeOption0, 0xE70F);
}

void enableADC() {
  writeBQ25730(ADCOption, 0x40FF);
  delay(50);
}

// -----------------------------
// Data Readouts
// -----------------------------

void readBatteryCurrent() {
  uint16_t raw = readBQ25730(ADCIBAT);

  // Bits [0:6]: 7-bit discharge current @ 512 mA/LSB
  uint8_t dischargeCode = raw & 0x7F;
  float dischargeCurrent_mA = dischargeCode * 512.0f;

  // Bits [8:14]: 7-bit charge current @ 128 mA/LSB
  uint8_t chargeCode = (raw >> 8) & 0x7F;
  float chargeCurrent_mA = chargeCode * 128.0f;

  Serial.print("Battery Discharge Current: ");
  Serial.print(dischargeCurrent_mA);
  Serial.print(" mA, Charge Current: ");
  Serial.print(chargeCurrent_mA);
  Serial.println(" mA");
}

// -----------------------------
// Debug Utility
// -----------------------------

void dumpAllRegisters() {
  for (uint8_t addr = 0x00; addr <= 0x3E; addr += 2) {
    uint16_t value = readBQ25730(addr);
    Serial.print("Reg 0x");
    if (addr < 0x10) Serial.print("0");
    Serial.print(addr, HEX);
    Serial.print(" = 0x");
    if (value < 0x1000) Serial.print("0");
    if (value < 0x0100) Serial.print("0");
    if (value < 0x0010) Serial.print("0");
    Serial.println(value, HEX);
  }
}


