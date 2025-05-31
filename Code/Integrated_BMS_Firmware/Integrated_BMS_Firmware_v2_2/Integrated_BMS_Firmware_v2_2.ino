//Integrated BMS Firmware V2_1
//Sodium & Lithium ION BMS

//Include wire for I2C
#include <Wire.h>
//Includes for INA
#include <Adafruit_INA260.h>
//include for interrupts
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>
#include "stm32f1xx.h"
#include <EEPROM.h>

//EEPROM Storage for Init System Values Passed by Controller uC
#define EEPROM_ADDR_1  0                                       // float 1
#define EEPROM_ADDR_2  (EEPROM_ADDR_1 + sizeof(float))         // float 2
#define EEPROM_ADDR_3  (EEPROM_ADDR_2 + sizeof(float))         // float 3
#define EEPROM_ADDR_4  (EEPROM_ADDR_3 + sizeof(float))         // float 4
#define EEPROM_ADDR_5  (EEPROM_ADDR_4 + sizeof(float))         // float 5
#define EEPROM_ADDR_6  (EEPROM_ADDR_5 + sizeof(float))         // float 6
#define EEPROM_ADDR_7  (EEPROM_ADDR_6 + sizeof(float))         // float 7
#define EEPROM_ADDR_8  (EEPROM_ADDR_7 + sizeof(float))         // float 8

////////////////////////////////////////////////////////////////
// Create instances for each INA260 sensor
Adafruit_INA260 ina260_0x40;
Adafruit_INA260 ina260_0x41;
Adafruit_INA260 ina260_0x44;

//Major system flags
volatile bool CHARGING = false;
String serialCmdBuffer = "";    // accumulator for incoming chars for Charge control
volatile bool CHARGE_ON_PLUGIN = true;  // when true, charging auto‑starts on plugin
volatile bool STOPCHARGING = false;
volatile bool KILLLOAD = false;
volatile bool PLUGGEDIN = false;
volatile bool OVERHEAT = false;
volatile bool ERRORFLG = false;

//Timer Flags
volatile bool TEST_FLAG = false;
volatile bool CONFIGURED = false;

int configstatus = 0;
int ConfigStatusFromMaster;
// Create a timer instance using TIM2 (adjust if needed)
STM32Timer ITimer2(TIM2);

// Timer ISR: set flags for sensor and serial update
void onTimerISR() {
  TEST_FLAG = true;
}


//INA219/260 & BQ25730 vars
float INA_0x40_VOLTAGE = 0.0;
float INA_0x41_VOLTAGE = 0.0;
float INA_0x44_VOLTAGE = 0.0;
float CELL1_VOLTAGE_LTC2943 = 0.00;
float CELL1_VOLTAGE = 0.00;
float CELL2_VOLTAGE = 0.00;
float CELL3_VOLTAGE = 0.00;
float CELL4_VOLTAGE = 0.00;

// 3.3V / 2 = 1.65V --> scaled to 12-bit ADC: (1.65 / 3.3) * 4095 ≈ 2047
const int THERMISTER_THRESHOLD = 2047;

//LTC scaling factors
//resistance value for LTC2943
float resistor = .1;
const float LTC2943_CHARGE_lsb = 0.34E-3;
const float LTC2943_VOLTAGE_lsb = 1.44E-3;
const float LTC2943_CURRENT_lsb = 29.3E-6;
const float LTC2943_TEMPERATURE_lsb = 0.25;
const float LTC2943_FULLSCALE_VOLTAGE = 23.6;
const float LTC2943_FULLSCALE_CURRENT = 60E-3;
const float LTC2943_FULLSCALE_TEMPERATURE = 510;
float prescaler = 4096;

//Balancer Declarations
float CellMaxCutOffV; //need from user --> Done
float CellFullChargeV = CellMaxCutOffV - 0.1;
float CellMinCutOffV = 2.0; //need from user --> Done

//vars for returns to main system
float PackVoltage = 0;
float current = 0;
float tempC = 0;
float charge_mAh = 0;
float real_charge_mAh = 0;

//LTC2943 Sense scaling Configuration parameters
float Pack_stock_capacity = 5200; //mA (2600*4 = 10400) //need from user --> Done
// Sense resistor in Ohms
const float RSENSE = 0.1f;

//CONTROL register prescaler setting (bits B[5:3] = 0b111 → M = 16384)
const float LTC2943_PRESCALER = 16384.0f;

//qLSB in mAh/count, per datasheet p.15:
//qLSB = 0.340 mAh × (50 mΩ / RSENSE) × (M / 4096)
const float QLSB = 0.340f * (50e-3f / RSENSE) * (LTC2943_PRESCALER / 4096.0f);

int numOfCells = 4;

//BQ25730 Configuration parameters
float ChargeVoltageValue=18;         // Charging target voltage - Later Change to --> float ChargeVoltageValue  = (numOfCells * CellMaxCutOffV) + 1;
float VsysMinValue;                 // Minimum system voltage
float inputVoltageValue;            // Input voltage limit (used for VINDPM)
float chargeCurrentValue=1;           // Battery charging current (in amps)
float inputCurrentLimitValue=1;    // Input current limit (in milliamps) - based on supply/adapter limit

unsigned long lastCheck1 = 0;
const unsigned long I2CcheckInterval = 5000;

unsigned long lastCheck2 = 0;
const unsigned long registerUpdateInterval = 2500;

//////////////////////////////////////////////////////////////////////

//Define Slave Address of STM32 for controller comms
float responseData = 0.0;
TwoWire myI2C2(PB11, PB10);
#define SLAVE_ADDRESS 0x58

//Adress for LTC2943
#define LTC2943_ADDR 0x64  // 7-bit I2C address
//Adress for BQ25730
#define BQ25730_ADDR 0x6B  // 7-bit I2C address

// LTC2943 Register Addresses
#define REG_STATUS     0x00
#define REG_CONTROL    0x01
#define REG_ACC_CHARGE_MSB 0x02
#define REG_VOLTAGE_MSB    0x08
#define REG_CURRENT_MSB    0x0E
#define REG_TEMPERATURE_MSB 0x14

// BQ25730 Register addresses
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
//////////////////////////////////////////////////////////////////////

//LTC Functions 
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
  float LTCvoltage = 23.6 * vRaw / 65535.0;
  return LTCvoltage;
}

float Request_Current_LTC2943(){
  // Current (12-bit, ±60mV full scale, offset binary)
  uint16_t iRaw = Read_LTC2943_Register(REG_CURRENT_MSB);
  int16_t iOffset = iRaw - 32767;
  float LTCcurrent = (60.0 / RSENSE) * iOffset / 32767.0;
  return LTCcurrent;
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
  //float charge_mAh = (acr - 32767) * QLSB;


  charge_mAh = 1000*(float)(acr*LTC2943_CHARGE_lsb*prescaler*50E-3)/(resistor*4096);
//
  return charge_mAh;
}

void setupLTC2943() {
  // Reset charge accumulator
  write_LTC2943_Register(REG_CONTROL, 0b11111001); // Set bit 0 to 1 to reset
  delay(1);
  write_LTC2943_Register(REG_CONTROL, 0b11111000); // Re-enable analog section, auto mode, prescaler 4096
}


//////////////////////////////////////////////////////////////////////
//BQ Functions 
void CheckBQConnectionWithComms(){
if (!checkBQConnection()) {
    Serial.println("BQ25730 not responding on I2C. Check wiring or power.");
    while (1); // Halt system
  } else {
    Serial.println("BQ25730 connection verified.");
  }
}
void ConfigAndStartBQ(){
  // Configure initial charger settings
  setChargeVoltage(ChargeVoltageValue);
  setInputVoltage(inputVoltageValue);
  setChargeCurrent(chargeCurrentValue);
  setInputCurrentLimit(inputCurrentLimitValue);
  set_VSYS_MIN(VsysMinValue);
  enableADC();
  enableCharging();
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

void disableVSYS() {
  Serial.println("------------- VSYS HAS BEEN DISABLED BECAUSE OF LOW BATTERY -----------------");

    // Step 1: Ensure OTG is disabled
    uint16_t chargeOption0 = readBQ25730(0x12);
    chargeOption0 &= ~(1 << 5);  // Clear EN_OTG bit
    writeBQ25730(0x12, chargeOption0);

    // Step 2: Ensure BATFETOFF_HIZ is cleared
    uint16_t chargeOption3 = readBQ25730(0x34);
    chargeOption3 &= ~(1 << 1);  // Clear BATFETOFF_HIZ
    writeBQ25730(0x34, chargeOption3);

    // Step 3: Force BATFET off
    chargeOption3 |= (1 << 7);   // Set BATFET_ENZ bit
    writeBQ25730(0x34, chargeOption3);

    // Step 4: Confirm
    chargeOption3 = readBQ25730(0x34);
    Serial.print("ChargeOption3 after BATFET_ENZ set: ");
    Serial.println(chargeOption3, HEX);

}


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
  Serial.println("");
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

void dumpAllBQRegisters() {
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

//Write to charge register to stop failsafe from lack of comms
void MaintainChargingBQ(){
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
    //dumpAllRegisters();
  }
  
}


//INA Functions
//Check if INA260s are connected
void CheckIfINAConnected()
{
   if (!ina260_0x40.begin(0x40)) {
    Serial.println("Couldn't find INA260 at 0x40");
    while (1);
  }
  if (!ina260_0x41.begin(0x41)) {
    Serial.println("Couldn't find INA260 at 0x41");
    while (1);
  }
  if (!ina260_0x44.begin(0x44)) {
    Serial.println("Couldn't find INA260 at 0x44");
    while (1);
  }
  /*
  if (!ina260_0x45.begin(0x45)) {
    Serial.println("Couldn't find INA260 at 0x45");
    while (1);
  } */
}

//Enables GPIO pins for balancer, CHGOK, LEDS, 
void EnableGPIOPins(){
  //Balancer bypass GPIO bank
  //Cell 1 Balance
  pinMode(PB1, OUTPUT);
  //Cell 2 Balance
  pinMode(PB0, OUTPUT);
  //Cell 3 Balance
  pinMode(PA7, OUTPUT);
  //Cell 4 Balance
  pinMode(PA6, OUTPUT);
  //CHG_OK
  pinMode(PA1, INPUT);

  //LED bank
  //PowerPlugInsertedLED
  pinMode(PB15, OUTPUT);//28
  //ChargeLED
  pinMode(PB14, OUTPUT);//27
  //BMS_ERROR_LED
  pinMode(PB13, OUTPUT);//26
  //BMS_STATUS_LED
  pinMode(PB12, OUTPUT);//25

  //Thermister bank
  //Thermister  1
  pinMode(PA2,INPUT);
  //Thermister  2
  pinMode(PA3,INPUT);
  //Thermister  3
  pinMode(PA4,INPUT);
  //Thermister  4
  pinMode(PA5,INPUT);
}
// Call this each loop to automatically stop charging if any cell exceeds charge or discharge conditions

void checkCellandSocCutoff() { 
  // Gather which cells are over the hard cutoff
  bool anyOverCutoff = false;
  String overList = "";
  if (CELL1_VOLTAGE_LTC2943 >= CellMaxCutOffV) { anyOverCutoff = true; overList += "1 "; }
  if (CELL2_VOLTAGE         >= CellMaxCutOffV) { anyOverCutoff = true; overList += "2 "; }
  if (CELL3_VOLTAGE         >= CellMaxCutOffV) { anyOverCutoff = true; overList += "3 "; }
  if (CELL4_VOLTAGE         >= CellMaxCutOffV) { anyOverCutoff = true; overList += "4 "; }

// Gather which cells are under the hard cutoff
  bool anyUnderCutoff = false;
  String underList = "";
  if (CELL1_VOLTAGE_LTC2943 <= CellMinCutOffV) { anyUnderCutoff = true; underList += "1 "; }
  if (CELL2_VOLTAGE         <= CellMinCutOffV) { anyUnderCutoff = true; underList += "2 "; }
  if (CELL3_VOLTAGE         <= CellMinCutOffV) { anyUnderCutoff = true; underList += "3 "; }
  if (CELL4_VOLTAGE         <= CellMinCutOffV) { anyUnderCutoff = true; underList += "4 "; }

  // Check if *all* cells are above the balancing voltage (CellFullChargeV)
  bool allAboveMax =
       (CELL1_VOLTAGE_LTC2943 >= CellFullChargeV) &&
       (CELL2_VOLTAGE         >= CellFullChargeV) &&
       (CELL3_VOLTAGE         >= CellFullChargeV) &&
       (CELL4_VOLTAGE         >= CellFullChargeV);


  charge_mAh = Request_SoC_LTC2943();
  //Adjust Charge to compensate for how LTC tracks Coulombs
  real_charge_mAh = charge_mAh+Pack_stock_capacity;

  //Any cell charged over CellMaxCutOffV stop charging
  if (anyOverCutoff && CHARGING) {
    disableCharging();
    CHARGING = false;
    Serial.println(F("\n=== CHARGING STOPPED: HARD OV CUTOFF ==="));
    Serial.print  (F("Cells over cutoff ("));
    Serial.print  (CellMaxCutOffV, 3);
    Serial.print  (F(" V): "));
    Serial.println(overList);  
    Serial.println(F("===========================================\n"));
  }

  //Any cell falls under anyUnderCutoff stop discharging
  if (anyUnderCutoff) {
    //STOPDISCHARGING();
    Serial.println(F("\n=== DISCHARGING STOPPED: HARD UV CUTOFF ==="));
    Serial.print  (F("Cells under cutoff ("));
    Serial.print  (CellMinCutOffV, 2);
    Serial.print  (F(" V): "));
    Serial.println(underList);  
    Serial.println(F("=============================================\n"));
    
  }

  // All cells charged stop charing
   if (allAboveMax && CHARGING) {
    disableCharging();
    CHARGING = false;
    Serial.println(F("\n=== CHARGING STOPPED: ALL CELLS ABOVE CellFullChargeVolt ==="));
    Serial.print  (F("All cells ≥ "));
    Serial.print  (CellFullChargeV, 2);
    Serial.println(F(" V"));
    Serial.println(F("=============================================================\n"));
  }

  //SoC says pack is charged stop charging
  if (real_charge_mAh >= Pack_stock_capacity) {
    disableCharging();
    CHARGING = false;
    Serial.println(F("\n=== CHARGING STOPPED: Pack is Fully Charged (SoC) ==="));
    Serial.print  (F("Pack at:"));
    Serial.print  (real_charge_mAh);
    Serial.println(F(" mAh"));
    Serial.println(F("=======================================================\n"));
  }

  //Disable DisCharging if SoC is empty
  if (real_charge_mAh <= 0 && CHARGING == false) {
    //STOPDISCHARGING();
    Serial.println(F("\n=== DISCHARGING STOPPED: Pack is EMPTY (SoC) ==="));
    Serial.print  (F("Pack at:"));
    Serial.print  (real_charge_mAh);
    Serial.println(F(" mAh"));
    Serial.println(F("============================================\n"));
    //disableVSYS();
    
  }
}

void ChargeAndBalanceControl(){
  if (CHARGING && !CHARGE_ON_PLUGIN) {
    Serial.println(F("⚠️  Auto Charge off, ENDING CHARGING"));
      // bail out of charging logic but still report voltages
      CHARGING = false;
      disableCharging();
  }

  if (CHARGE_ON_PLUGIN) {
    if (digitalRead(PA1) == HIGH) {
      Serial.println(F("⚠️  Auto charge on: Charging"));
      CHARGING = true;
      PLUGGEDIN = true;
    }
    else
    {
      Serial.println(F("⚠️  Auto charge on: Please plug in charger to start balancing/charging."));
      CHARGING = false;
      PLUGGEDIN = false;
      disableCharging();
    }
  }
  
  //Current (12-bit, ±60mV full scale, offset binary)
  current = Request_Current_LTC2943();

  disableCharging();
  delay(400);
  //Grab each cells Voltage
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage()/1000.00;
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage()/1000.00;
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage()/1000.00;

  //Calulate Cell Voltages based off of INA readings
  CELL1_VOLTAGE_LTC2943 = Request_Voltage_LTC2943()- INA_0x44_VOLTAGE;
  CELL2_VOLTAGE = INA_0x44_VOLTAGE - INA_0x41_VOLTAGE;
  CELL3_VOLTAGE = INA_0x41_VOLTAGE - INA_0x40_VOLTAGE;
  CELL4_VOLTAGE = INA_0x40_VOLTAGE;
  

  //Check if any Cells are overvolted
  checkCellandSocCutoff();
  
  //Voltage (16-bit, 23.6V full-scale)
  PackVoltage = Request_Voltage_LTC2943();
  
  //Temperature (11-bit, 510K full-scale)
  tempC = Request_Temp_LTC2943();
  
  //Accumulated Charge
  charge_mAh = Request_SoC_LTC2943();
  

  //Adjust Charge to compensate for how LTC tracks Coulombs
  real_charge_mAh = charge_mAh+Pack_stock_capacity;
  

  if(CHARGING == true)
  {
    enableCharging();
    MaintainChargingBQ();
    //If cell 1 outside of CellFullChargeV Activate Balancer on Cell 1
    if(CELL1_VOLTAGE_LTC2943 >= CellFullChargeV){
      Serial.print("CELL 1 Voltage: ");
      Serial.println(CELL1_VOLTAGE_LTC2943);
      //Serial.print("CELL 1 (INA) Voltage: ");
      //Serial.println(CELL1_VOLTAGE);
      Serial.print("CELL 1 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PB1, HIGH);
    }
    else{
      Serial.print("CELL 1 Voltage: ");
      Serial.println(CELL1_VOLTAGE_LTC2943);
      //Serial.print("CELL 1 (INA) Voltage: ");
      //Serial.println(CELL1_VOLTAGE);
      Serial.print("Cell 1 in balance\n");
      digitalWrite(PB1, LOW);
    }
    //If cell 2 outside of CellFullChargeV Activate Balancer on Cell 2
    if(CELL2_VOLTAGE >= CellFullChargeV)
    {
      Serial.print("CELL 2 Voltage: ");
      Serial.println(CELL2_VOLTAGE);
      Serial.print("CELL 2 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PB0, HIGH);
    }
    else{
      Serial.print("CELL 2 Voltage: ");
      Serial.println(CELL2_VOLTAGE);
      Serial.print("Cell 2 in balance\n");
      digitalWrite(PB0, LOW);
    }
    //If cell 3 outside of CellFullChargeV Activate Blancer on Cell 3
    if(CELL3_VOLTAGE >= CellFullChargeV)
    {
      Serial.print("CELL 3 Voltage: ");
      Serial.println(CELL3_VOLTAGE);
      Serial.print("CELL 3 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PA7, HIGH);
    }
    else{
      Serial.print("CELL 3 Voltage: ");
      Serial.println(CELL3_VOLTAGE);
      Serial.print("Cell 3 in balance\n");
      digitalWrite(PA7, LOW);
    }
    //If cell 4 outside of CellFullChargeV Activate Blancer on Cell 4
    //USING LTC_2943
    if(CELL4_VOLTAGE >= CellFullChargeV)
    {
      Serial.print("CELL 4 Voltage: ");
      Serial.println(CELL4_VOLTAGE);
      Serial.print("CELL 4 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PA6, HIGH);
      Serial.println("");
    }
    else{
      Serial.print("CELL 4 Voltage: ");
      Serial.println(CELL4_VOLTAGE);
      Serial.print("Cell 4 in balance\n");
      digitalWrite(PA6, LOW);
      Serial.println("");
    }

  }
  else{
    Serial.print("CELL 1 (LTC) Voltage: ");
    Serial.println(CELL1_VOLTAGE_LTC2943);
    //Serial.print("CELL 1 (INA) Voltage: ");
    //Serial.println(CELL1_VOLTAGE);
    Serial.print("CELL 2 Voltage: ");
    Serial.println(CELL2_VOLTAGE);
    Serial.print("CELL 3 Voltage: ");
    Serial.println(CELL3_VOLTAGE);
    Serial.print("CELL 4 Voltage: ");
    Serial.println(CELL4_VOLTAGE);
    Serial.println("");

 
  }
//Print out LTC status
  Serial.println("LTC2943 Measurements:");
  Serial.print("Pack Voltage: "); Serial.print(PackVoltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(current, 2); Serial.println(" mA");
  Serial.print("Temperature: "); Serial.print(tempC); Serial.println(" °C");
  Serial.print("Accumulated Charge: "); Serial.print(charge_mAh); Serial.println(" mAh");
  Serial.print("Actual Accumulated Charge: "); Serial.print(real_charge_mAh); Serial.println(" mAh");
  Serial.println();
}


void handleSerialCharging() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;            // ignore CR
    if (c == '\n') {                    // full line received
      String cmd = serialCmdBuffer;
      cmd.trim();
      cmd.toUpperCase();

      if (cmd == "START") {
        CHARGING = true;
        Serial.println(F("⚡ Charging ENABLED"));
      }
      else if (cmd == "STOP") {
        CHARGING = false;
        Serial.println(F("⛔ Charging DISABLED"));
      }
      else if (cmd == "AUTO") {
        CHARGE_ON_PLUGIN = !CHARGE_ON_PLUGIN;
        Serial.print(F("🔄 Auto‑charge on plugin "));
        Serial.println(CHARGE_ON_PLUGIN ? F("ENABLED") : F("DISABLED"));
      }
      else {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        Serial.println(F("  Use START, STOP or AUTO"));
      }

      serialCmdBuffer = "";             // clear for next line
    }
    else {
      serialCmdBuffer += c;             // accumulate chars
      // cap buffer length
      if (serialCmdBuffer.length() > 20)
        serialCmdBuffer = serialCmdBuffer.substring(serialCmdBuffer.length() - 20);
    }
  }
}

void TempCheck() {
  // Read analog values
  int t1 = analogRead(PA2);
  int t2 = analogRead(PA3);
  int t3 = analogRead(PA4);
  int t4 = analogRead(PA5);

  // Check if any reading exceeds threshold
  if(t1 > THERMISTER_THRESHOLD || t2 > THERMISTER_THRESHOLD || t3 > THERMISTER_THRESHOLD || t4 > THERMISTER_THRESHOLD)
  {
    Serial.print("THERMISTER ERROR FLAG ON Voltages:"); Serial.print(t1); Serial.print(t2); Serial.print(t3); Serial.println(t4);
    disableCharging();
    ERRORFLG = true;
    OVERHEAT = true;
    disableVSYS();
  }
}
void ConfigCheck(){
  Serial.print("Config status:");Serial.print(configstatus);Serial.println("/8");
  if(configstatus >= 7){
    CONFIGURED = true;
    Serial.println("Namaste");
    ITimer2.attachInterruptInterval(5000000, SystemCheck);
  }
  else{
    CONFIGURED = false;
    Serial.println("Fuck you");
    ITimer2.attachInterruptInterval(5000000, SystemCheck);
  }


}

// Handle command + optional float reception
void onReceive(int howMany) {
  
  if (howMany == 1) {
    uint8_t command = myI2C2.read();
    switch (command) {
      case 0x01:
        CELL1_VOLTAGE = PackVoltage - INA_0x44_VOLTAGE;
        responseData = CELL1_VOLTAGE;
        break;
      case 0x02:
        CELL2_VOLTAGE = INA_0x44_VOLTAGE - INA_0x41_VOLTAGE;
        responseData = CELL2_VOLTAGE;
        break;
      case 0x03:
        CELL3_VOLTAGE = INA_0x41_VOLTAGE - INA_0x40_VOLTAGE;
        responseData = CELL3_VOLTAGE;
        break;
      case 0x04:
        CELL4_VOLTAGE = INA_0x40_VOLTAGE;
        responseData = CELL4_VOLTAGE;
        break;
      case 0x05:
        responseData = PackVoltage;
        break;
      case 0x06:
        responseData = current;
        break;
      case 0x08:
        responseData = real_charge_mAh;
        break;
      default:
        responseData = -1.0;
        break;
    }
  } else if (howMany == 5) {
    uint8_t command = myI2C2.read();
    uint8_t buffer[4];
    for (int i = 0; i < 4; i++) {
      buffer[i] = myI2C2.read();
    }
    float receivedFloat;
    memcpy(&receivedFloat, buffer, sizeof(float));

    Serial.print("Received float command 0x");
    Serial.print(command, HEX);
    Serial.print(" with value: ");
    Serial.println(receivedFloat, 5);
    
    //Pause timer to avoid collision
    ITimer2.detachInterrupt();
    switch (command) {
      case 0x10:
        ChargeVoltageValue = receivedFloat;
        Serial.print("ChargeVoltageValue Recieved From Peripheral:  ");Serial.println(ChargeVoltageValue);
        EEPROM.put(EEPROM_ADDR_1, ChargeVoltageValue);
        configstatus++;
        break;
      case 0x11:
        VsysMinValue = receivedFloat;
        Serial.print("VsysMinValue Recieved From Peripheral:  ");Serial.println(VsysMinValue);
        EEPROM.put(EEPROM_ADDR_2, VsysMinValue);
        configstatus++;
        break;
      case 0x12:
        VsysMinValue = receivedFloat;
        Serial.print("VsysMinValue Recieved From Peripheral:  ");Serial.println(VsysMinValue);
        configstatus++;
        break;
      case 0x13:
        inputVoltageValue = receivedFloat;
        Serial.print("inputVoltageValue Recieved From Peripheral:  ");Serial.println(inputVoltageValue);
        EEPROM.put(EEPROM_ADDR_3, inputVoltageValue);
        configstatus++;
        break;
      case 0x14:
        chargeCurrentValue = receivedFloat;
        Serial.print("chargeCurrentValue Recieved From Peripheral:  ");Serial.println(chargeCurrentValue);
        EEPROM.put(EEPROM_ADDR_4, chargeCurrentValue);
        configstatus++;
        break;
      case 0x15:
        inputCurrentLimitValue = receivedFloat;
        Serial.print("inputCurrentLimitValue Recieved From Peripheral:  ");Serial.println(inputCurrentLimitValue);
        EEPROM.put(EEPROM_ADDR_5, inputCurrentLimitValue);
        configstatus++;
        break;
      case 0x16:
        CellMaxCutOffV = receivedFloat;
        CellFullChargeV = CellMaxCutOffV - 0.1;
        Serial.print("CellMaxCutOffV Recieved From Peripheral:  ");Serial.println(CellMaxCutOffV);
        EEPROM.put(EEPROM_ADDR_6, CellMaxCutOffV);
        configstatus++;
        break;
      case 0x17:
        CellMinCutOffV = receivedFloat;
        Serial.print("CellMinCutOffV Recieved From Peripheral:  ");Serial.println(CellMinCutOffV);
        EEPROM.put(EEPROM_ADDR_7, CellMinCutOffV);
        configstatus++;
        break;
      case 0x18:
        Pack_stock_capacity = receivedFloat;
        Serial.print("Pack_stock_capacity Recieved From Peripheral:  ");Serial.println(Pack_stock_capacity);
        EEPROM.put(EEPROM_ADDR_8, Pack_stock_capacity);
        configstatus++;
        ConfigCheck();
        break;
        case 0x19:
        ConfigStatusFromMaster = receivedFloat;
        Serial.print("Configuration status from Master: ");Serial.println(ConfigStatusFromMaster);
        if(ConfigStatusFromMaster == 1)
        {
          Serial.println("Master reported Configuration complete");
          configstatus = 7;
          
          ConfigCheck();
        }
        break;
      default:
        Serial.println("Unknown float write command");
        break;
    }
  }
}

// Send response back to master
void onRequest() {
  myI2C2.write((uint8_t *)&responseData, sizeof(responseData));
  responseData = 0.0;
}


void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(3000);
  Wire.begin();
  delay(100);

  myI2C2.begin(SLAVE_ADDRESS);
  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);


  setupLTC2943();
  setupLTC2943();
  setupLTC2943();
  CheckBQConnectionWithComms();
  CheckIfINAConnected();
  EnableGPIOPins();
  //printSerialMenu();

  // Start the timer interrupt to trigger every 500,000 microseconds (5000 ms)
  if (ITimer2.attachInterruptInterval(5000000, SystemCheck)) {
    Serial.println("Timer2 started, checking sensors every 500 ms");
  } else {
    Serial.println("Failed to start Timer2");
  }
}

//SystemCheck is Basically the main() loop, gets called every 500ms by timer
void SystemCheck()
{
  Serial.println("----------------------------------------------------");
  //LTC2943 loop code 
  // Keep analog section active if Pack is disconnected temporarily
  //write_LTC2943_Register(REG_CONTROL, 0b11111000);
  if(CONFIGURED == true){

    //Grab user Input
  handleSerialCharging();
  //Print Charging status
  Serial.print("Charging: ");Serial.println(CHARGING);
  ChargeAndBalanceControl();
  TempCheck();

  }
  else{
    Serial.println("        AWAITING CONFIGURATION");
    //Serial.print("Config Flag:");Serial.println(CONFIGURED);
    //Serial.print("Config status:");Serial.print(configstatus);Serial.println("/8");
  }
  

  Serial.println("----------------------------------------------------");
  Serial.println("");
}

void loop() {
  __asm__("nop");
  /*Serial.print("Line 921 -- Inside loop()................n\n\n\n\n");
  //Serial.print("TEST_FLAG Value = ");Serial.println(TEST_FLAG);
  if(TEST_FLAG){
    Serial.print("\n\n\n---- In TEST_FLAG if------\n\n\n");
    TEST_FLAG = false;
    SystemCheck();
  }*/
}
//EOF