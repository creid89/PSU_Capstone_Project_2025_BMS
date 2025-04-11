#include <Wire.h>

#define BQ25730_ADDR 0x6B  // 7-bit I2C address

// register addresses
#define CHARGE_OPTION_0  0x00
#define CHARGE_CURRENT   0x02
#define CHARGE_VOLTAGE   0x04
#define OTG_VOLTAGE      0x06
#define OTG_CURRENT      0x08
#define INPUT_VOLTAGE    0x0A  
#define VSYS_MIN         0x0C  //minimum system voltage setting
#define IIN_HOST         0x0E  //input current limit
#define CHARGER_STATUS   0x20
#define PROCHOT_STATUS   0x22
#define IIN_DPM          0x24
#define ADC_VBUS_PSYS    0x26
#define ADC_IBAT         0x28
#define ADC_IIN_CMPIN    0x2A 
#define ADC_VSYS_VBAT    0x2C 
#define MANUFACTURER_ID  0x2E
#define DEVICE_ID        0x2F
#define CHARGE_OPTION_3  0x34




 void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("BQ25730 Control");
 

  // Initialize charger settings
  setChargeVoltage(14800); // 4 cells * 3.7V = 14.8 (in mV)
  setChargeCurrent(1000);  // 1000mA = 1A
  setInputCurrentLimit(2000); // 2000mA = 2A
  setSystemVoltage(12000); // 12000mV = 12V
  enableCharging();
 }
 

 void loop() {
  // Monitor battery voltage and current (example)
  uint16_t batteryVoltage = readBQ25730(ADC_VSYS_VBAT); // read pack voltage
  uint16_t iBatCurrent = readBQ25730(ADC_IBAT);  // Replace with actual IBAT register
  uint16_t chargeCurrent = readBQ25730(CHARGE_CURRENT);
  uint16_t status = readBQ25730(CHARGER_STATUS);
 

  Serial.print("VBAT: ");
  Serial.print(batteryVoltage);
  Serial.print(" mV, IBAT: ");
  Serial.print(iBatCurrent);
  Serial.print(" mA, Charge Status: 0x");
  Serial.println(status, HEX);
  Serial.print("CHARGE_CURRENT Reg Value: 0x");
  Serial.println(chargeCurrent, HEX);



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
  writeBQ25730(CHARGE_VOLTAGE, voltage_mV);
 }
 

 void setChargeCurrent(uint16_t current_mA) {
  writeBQ25730(CHARGE_CURRENT, current_mA);
 }
 

 void setInputCurrentLimit(uint16_t current_mA) {
  writeBQ25730(IIN_HOST, current_mA);
 }
 

 void setSystemVoltage(uint16_t voltage_mV) {
  writeBQ25730(VSYS_MIN, voltage_mV);
 }
 

 void enableCharging() {
  uint16_t option0 = readBQ25730(CHARGE_OPTION_0);
  //option0 |= (1 << 0);  // Example: Set the Charge Enable bit
  option0 = 0xE70E; 
  writeBQ25730(CHARGE_OPTION_0, option0);
 }
 

 void disableCharging() {
  uint16_t option0 = readBQ25730(CHARGE_OPTION_0);
  option0 &= ~(1 << 0); // Example: Clear the Charge Enable bit
  writeBQ25730(CHARGE_OPTION_0, option0);
 }