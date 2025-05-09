//LTC2943 Driver V0.3
//Sodium ION BMS

//Include wire for I2C
#include <Wire.h>

//Adress for LTC2943
#define LTC2943_ADDR 0x64  // 7-bit I2C address

// LTC2943 Register Addresses
#define REG_STATUS     0x00
#define REG_CONTROL    0x01
#define REG_ACC_CHARGE_MSB 0x02
#define REG_VOLTAGE_MSB    0x08
#define REG_CURRENT_MSB    0x0E
#define REG_TEMPERATURE_MSB 0x14


//////////////////////////////////////////////////////////////////////
// Sense resistor value in ohms (as measured)
const float RSENSE = 0.1;  // Ohms
const float QLSB = 0.340 * 0.050 / RSENSE;  // mAh
//////////////////////////////////////////////////////////////////////


//write to LTC2943 register over I2C
void write_LTC2943_Register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LTC2943_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

//Reqest register and return register contents over I2C
uint16_t Read_LTC2943_Register(uint8_t reg) {
  Wire.beginTransmission(LTC2943_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LTC2943_ADDR, (uint8_t)2);
  if (Wire.available() < 2) {
    Serial.println("I2C read failed");
    return 0;
  }
  uint16_t value = Wire.read() << 8;
  value |= Wire.read();
  return value;
}
//Functions to Modularly Reqest info from Fuel Guage
float Request_Voltage_LTC2943(){
  uint16_t vRaw = Read_LTC2943_Register(REG_VOLTAGE_MSB);
  // Voltage (16-bit, 23.6V full-scale)
  float voltage = 23.6 * vRaw / 65535.0;
  return voltage;
}

float Request_Current_LTC2943(){
  // Current (12-bit, ±60mV full scale, offset binary)
  uint16_t iRaw = Read_LTC2943_Register(REG_CURRENT_MSB);
  int16_t iOffset = iRaw - 32767;
  float current = (60.0 / RSENSE) * iOffset / 32767.0;
  return current;
}

float Request_Temp_LTC2943(){
    uint16_t tRaw = Read_LTC2943_Register(REG_TEMPERATURE_MSB);
  float tempK = 510.0 * tRaw / 65535.0;
  float tempC = tempK - 273.15;
  return tempC;
}

float Request_SoC_LTC2943(){
  // Accumulated Charge (SoC)
  uint16_t acr = Read_LTC2943_Register(REG_ACC_CHARGE_MSB);
  float charge_mAh = (acr - 32767) * QLSB;
  return charge_mAh;
}

void setupLTC2943() {
  // Reset charge accumulator
  write_LTC2943_Register(REG_CONTROL, 0b11111001); // Set bit 0 to 1 to reset
  delay(1);
  write_LTC2943_Register(REG_CONTROL, 0b11111000); // Re-enable analog section, auto mode, prescaler 4096
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(100);
  setupLTC2943();
}

void loop() {
  // Keep analog section active if Pack is disconnected temporarily
  write_LTC2943_Register(REG_CONTROL, 0b11111000);

  //Print out 
  Serial.println("LTC2943 Measurements:");

  // Voltage (16-bit, 23.6V full-scale)
  float voltage = Request_Voltage_LTC2943();
  Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");

  // Current (12-bit, ±60mV full scale, offset binary)
  float current = Request_Current_LTC2943();
  Serial.print("Current: "); Serial.print(current, 2); Serial.println(" mA");

  // Temperature (11-bit, 510K full-scale)
  float tempC = Request_Temp_LTC2943();
  Serial.print("Temperature: "); Serial.print(tempC); Serial.println(" °C");

  // Accumulated Charge
  float charge_mAh = Request_SoC_LTC2943();
  Serial.print("Accumulated Charge: "); Serial.print(charge_mAh); Serial.println(" mAh");

  Serial.println();
  delay(2000);
}