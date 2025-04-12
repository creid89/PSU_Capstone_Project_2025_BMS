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
#define ADCIBAT         0x28
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



 void setup() {
  Serial.begin(115200);
  delay(5000);
  Wire.begin();
  Serial.println("BQ25730 Control");
  
 

  // Initialize charger settings
  setChargeVoltage(0x4000); // 4 cells * 4V = 16 (in mV), 0x3F = 16.328V
  setInputVoltage(0x0800); // set minium input voltage as 5.2, there is a 3.2V offset
  setChargeCurrent(0x0200);  // 1000mA = 1A
  setInputCurrentLimit(0x2000); // 3200mA
  setSystemVoltage(0x9E00); // setting VSYS as 15.8V
  enableCharging();
 }
 

 void loop() {
  // Monitor battery voltage and current (example)
  uint16_t batteryVoltage = readBQ25730(ADCVSYSVBAT); // read pack voltage
  uint16_t iBatCurrent = readBQ25730(ADCIBAT);  // Replace with actual IBAT register
  uint16_t chargeCurrent = readBQ25730(ChargeCurrent);
  uint16_t status = readBQ25730(ChargerStatus);
  uint16_t inputCurrentLimt = readBQ25730(IIN_HOST);
  uint16_t inputVoltageLimt = readBQ25730(InputVoltage); 
  uint16_t batChargeVoltage = readBQ25730(ChargeVoltage); 

 

  Serial.print("VBAT: ");
  Serial.print(batteryVoltage);
  Serial.print(" mV, IBAT: ");
  Serial.print(iBatCurrent);
  Serial.print(" mA, chargeStatus Reg Value: 0x");
  Serial.println(status, HEX);
  Serial.print("chargeCurrent Reg Value: 0x");
  Serial.println(chargeCurrent, HEX);
  Serial.print("IIN_HOST Reg Value: 0x");
  Serial.println(inputCurrentLimt, HEX);
  Serial.print("InputVoltage Reg Value: 0x");
  Serial.println(inputVoltageLimt, HEX);
  Serial.print("ChargeVoltage Reg Value: 0x");
  Serial.println(batChargeVoltage, HEX);
  delay(1000);
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
 

 // Charger control functions (modify based on datasheet)
 void setChargeVoltage(uint16_t voltage_mV) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(voltage_mV, HEX);Serial.println(" to ChargeVoltage Register");
  writeBQ25730(ChargeVoltage, voltage_mV);
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
 
 void setInputVoltage(uint16_t voltage_mV) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(voltage_mV, HEX);Serial.println(" to InputVoltage Register");
  writeBQ25730(InputVoltage, voltage_mV);
 }


 void setSystemVoltage(uint16_t voltage_mV) {
  delay(2500);
  Serial.print("Writing: ");Serial.print(voltage_mV, HEX);Serial.println(" to VSYS_MIN (Minimum System Voltage) Register");
  writeBQ25730(VSYS_MIN, voltage_mV);
 }
 

 void enableCharging() {
  uint16_t option0 = readBQ25730(ChargeOption0);
  //option0 |= (1 << 0);  // Example: Set the Charge Enable bit
  option0 = 0xE70E; 
  writeBQ25730(ChargeOption0, option0);
 }
 

 void disableCharging() {
  uint16_t option0 = readBQ25730(ChargeOption0);
  option0 &= ~(1 << 0); // Example: Clear the Charge Enable bit
  writeBQ25730(ChargeOption0, option0);
 }