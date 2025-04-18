#include <Wire.h>

#define BQ25730_ADDR 0x6B  // 7-bit I2C address

// register addresses
#define ChargeOption0   0x00
#define ChargeCurrent   0x02 // 7-bit charge current setting - R/W
#define ChargeVoltage   0x04 // 12-bit charge voltage setting - R/W
#define OTGVoltage      0x06 // 12-bit OTG voltage setting - R/W
#define OTGCurrent      0x08 // 12-bit OTG voltage setting - R/W
#define InputVoltage    0x0A // 8-bit input voltage setting - R/W
#define VSYS_MIN        0x0C // minimum system voltage setting - R/W
#define IIN_HOST        0x0E // input current limit, 6-bit Input current limit set by host - R/W
#define ChargerStatus   0x20
#define ProchotStatus   0x22
#define IIN_DPM         0x24 // 7-bit input current limit in use - R/W
#define AADCVBUS_PSYS   0x26 // 8-bit digital output of input voltage - 8-bit digital output of system power - Read Only
#define ADCIBAT         0x28 // 7-bit digital output of battery charge current & 7-bit digital output of battery discharge current
#define ADCIINCMPIN     0x2A // 7-bit digital output of battery charge current - 7-bit digital output of battery discharge current - Read Only
#define ADCVSYSVBAT     0x2C // 7-bit digital output of battery discharge current - 8-bit digital output of CMPIN voltage - Read Only
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
float ChargeVoltageValue = 17.000; //getting the pack to 16V, needs the charge voltage at 17.00V
float VsysMinValue = 12.3;
float inputVoltageValue = 5.2;

 void setup() {
  Serial.begin(115200);
  delay(5000);
  Wire.begin();
  Serial.println("BQ25730 Control");
  
 
  /*
  Setting ChargerVoltage and ChargerCurrent in the loop so they are set 
  after external power is disconnected and reconnected
  */

  /*
  Things to Do:
  -Finish function to enter floating points - the convert to uint16_t - writre to registers
  -read from current and voltage registers and convert to human readable from
  -read from ChargeStatus Registers and convert to human readable form
  -write to ChargeOption register to turn charging on and off --> this will be controlled by the end user???
  -
  */

  // Initialize charger settings
  setChargeVoltage(ChargeVoltageValue); // Set charge v
  setInputVoltage(inputVoltageValue); // set minium input voltage as 5.2, there is a 3.2V offset
  setChargeCurrent(0x0200);  // 1000mA = 1A
  setInputCurrentLimit(0x2000); // 3200mA
  set_VSYS_MIN(VsysMinValue); // Set minimum system voltage in incrments of 100mA between 1.0-23.0V
  enableADC();
  enableCharging();
 }
 

 void loop() {
  Serial.println("\n--- Register Dump ---");
  //enableADC();
  //enableCharging();
  delay(3000);
  setChargeVoltage(ChargeVoltageValue);
  setChargeCurrent(0x0200);  // 1000mA = 1A
  enableCharging();
  dumpAllRegisters();

  // Monitor battery voltage and current (example)
  /*uint16_t batteryVoltage = readBQ25730(ADCVSYSVBAT); 
  uint16_t iBatCurrent = readBQ25730(ADCIBAT);  //reading ADCIBAT register
  uint16_t chargeCurrent = readBQ25730(ChargeCurrent);
  uint16_t status = readBQ25730(ChargerStatus);
  uint16_t inputCurrentLimt = readBQ25730(IIN_HOST);
  uint16_t inputVoltageLimt = readBQ25730(InputVoltage); 
  uint16_t batChargeVoltage = readBQ25730(ChargeVoltage); 

 

  Serial.print("ADCVSYSVBAT Reigster Value: ");
  Serial.print(batteryVoltage, HEX);
  Serial.print(" , ADCIBAT Register Value: ");
  Serial.print(iBatCurrent, HEX);
  Serial.print(" , chargeStatus Reg Value: 0x");
  Serial.println(status, HEX);
  Serial.print("chargeCurrent Reg Value: 0x");
  Serial.println(chargeCurrent, HEX);
  Serial.print("IIN_HOST Reg Value: 0x");
  Serial.println(inputCurrentLimt, HEX);
  Serial.print("InputVoltage Reg Value: 0x");
  Serial.println(inputVoltageLimt, HEX);
  Serial.print("ChargeVoltage Reg Value: 0x");
  Serial.println(batChargeVoltage, HEX);
  delay(1000);*/
 }
 
 // Dump all BQ25730 registers
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

 // I2C write function
 void writeBQ25730(uint8_t regAddr, uint16_t data) {
  Wire.beginTransmission(BQ25730_ADDR);
  Wire.write(regAddr);
  Wire.write(data & 0xFF);      // Low byte
  Wire.write((data >> 8) & 0xFF); // High byte
  Wire.endTransmission();
  delay(5);
 }
 
 // I2C read function
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
 
 void enableADC() {
  // Enable continuous ADC with averaging
  writeBQ25730(ADCOption, 0x40FF);
  delay(100); // Give time for readings to stabilize
}


 // Charger control functions (modify based on datasheet)
 /*void setChargeVoltage(uint16_t voltage_mV) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(voltage_mV, HEX);Serial.println(" to ChargeVoltage Register");
  writeBQ25730(ChargeVoltage, voltage_mV);
 }*/

 void setChargeVoltage(float voltage_V) {
  // Voltage range check
  if (voltage_V < 1.024 || voltage_V > 23.000) {
    Serial.println("Charge voltage out of range (1.024V – 23.000V). Write skipped.");
    return;
  }

  // Convert to number of 8mV steps
  uint16_t stepCount = static_cast<uint16_t>(voltage_V / 0.008 + 0.5);  // Round to nearest step

  // Only keep bits [12:0] and shift into position [14:3]
  stepCount &= 0x1FFF;          // mask to 13 bits
  uint16_t encoded = stepCount << 3;  // shift into bits [14:3], reserved bits cleared

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to ChargeVoltage ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(ChargeVoltage, encoded);
}

 

 void setChargeCurrent(uint16_t current_mA) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(current_mA, HEX);Serial.println(" to ChargeCurrent Register");
  writeBQ25730(ChargeCurrent, current_mA);
 }
 

 void setInputCurrentLimit(uint16_t current_mA) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(current_mA, HEX);Serial.println(" to IIN_HOST (Input Current Limut) Register");
  writeBQ25730(IIN_HOST, current_mA);
 }
 
 /*void setInputVoltage(uint16_t voltage_mV) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(voltage_mV, HEX);Serial.println(" to InputVoltage Register");
  writeBQ25730(InputVoltage, voltage_mV);
 }*/

 void setInputVoltage(float voltage_V) {
  // Ensure voltage is above minimum offset
  if (voltage_V < 3.2 || voltage_V > 23.0) {
    Serial.println("Input voltage limit out of range (3.2V – 23.0V). Write skipped.");
    return;
  }

  // Remove 3.2V offset and convert to step count (64 mV per step)
  uint16_t stepCount = static_cast<uint16_t>((voltage_V - 3.2) / 0.064 + 0.5);  // Rounded

  // Place stepCount in bits [6:13] (shifted by 6), and mask reserved bits
  uint16_t encoded = (stepCount << 6) & 0x3FC0;  // Keep bits [6:13], clear [0:5] and [14:15]

  delay(2500);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to InputVoltage Register ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(InputVoltage, encoded);
}



 /*void set_VSYS_MIN(uint16_t voltage_mV) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(voltage_mV, HEX);Serial.println(" to VSYS_MIN (Minimum System Voltage) Register");
  writeBQ25730(VSYS_MIN, voltage_mV);
 }
 
uint16_t encodeVSYS_MIN(float voltage_V) {
  if (voltage_V < 1.0 || voltage_V > 23.0) return 0;  // Ignore invalid range
  return static_cast<uint16_t>(voltage_V * 10) << 8;  // Convert to 100mV steps, shift to bits [15:8]
}*/

void set_VSYS_MIN(float voltage_V) {
  if (voltage_V < 1.0 || voltage_V > 23.0) {
    Serial.println("VSYS_MIN voltage out of range (1.0V–23.0V). Write skipped.");
    return;
  }

  uint16_t encoded = static_cast<uint16_t>(voltage_V * 10) << 8;  // Convert to 100 mV steps, shift into bits [15:8]
  delay(2500);
  Serial.print("Writing: 0x"); Serial.print(encoded, HEX);
  Serial.print(" to VSYS_MIN ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(VSYS_MIN, encoded);
}



 void enableCharging() {
  uint16_t option0 = readBQ25730(ChargeOption0);
  //option0 |= (1 << 0);  // Example: Set the Charge Enable bit
  option0 = 0xE70E; //turned off watchdog, charging now stays on longer then 175 seconds 
  writeBQ25730(ChargeOption0, option0);
 }
 

 void disableCharging() {
  uint16_t option0 = readBQ25730(ChargeOption0);
  option0 &= ~(1 << 0); // Example: Clear the Charge Enable bit
  writeBQ25730(ChargeOption0, option0);
 }