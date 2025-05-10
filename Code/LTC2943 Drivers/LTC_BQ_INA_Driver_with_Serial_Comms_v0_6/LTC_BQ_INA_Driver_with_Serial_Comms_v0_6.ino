//LTC2943_and_BQ_and_INA_Driver_v0_6
//Sodium ION BMS

//Include wire for I2C
#include <Wire.h>
//Includes for INA
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>

////////////////////////////////////////////////////////////////
// Create instances for each INA260 sensor
Adafruit_INA260 ina260_0x40;
Adafruit_INA260 ina260_0x41;
Adafruit_INA260 ina260_0x44;
Adafruit_INA260 ina260_0x45;

//Used to toggle balancing
volatile bool CHARGING = false;
String serialCmdBuffer = "";    // accumulator for incoming chars for Charge control
volatile bool BALANCE = false;  //Used to toggle balancing

//Global Variable
float PACK_CURRENT;
float INA_0x40_VOLTAGE = 0.0;
float INA_0x41_VOLTAGE = 0.0;
float INA_0x44_VOLTAGE = 0.0;
float INA_0x45_VOLTAGE = 0.0;
float CELL1_VOLTAGE_LTC2943 = 0.00;
float CELL1_VOLTAGE = 0.00;
float CELL2_VOLTAGE = 0.00;
float CELL3_VOLTAGE = 0.00;
float CELL4_VOLTAGE = 0.00;

//Balancer Declarations
float CellVMax = 4.0;

///////////////////////////////////////////////////////////////////
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
//LTC2943 Sense scaling Configuration parameters
// Sense resistor value in ohms (as measured)
const float RSENSE = 0.1;  // Ohms
const float QLSB = 0.340 * 0.050 / RSENSE;  // mAh
//////////////////////////////////////////////////////////////////////
//BQ25730 Configuration parameters
float ChargeVoltageValue = 17.000;         // Charging target voltage
float VsysMinValue = 13.3;                 // Minimum system voltage
float inputVoltageValue = 12.0;            // Input voltage limit (used for VINDPM)
float chargeCurrentValue = 1.00;           // Battery charging current (in amps)
float inputCurrentLimitValue = 2100.00;    // Input current limit (in milliamps) - based on supply/adapter limit

unsigned long lastCheck1 = 0;
const unsigned long I2CcheckInterval = 5000;

unsigned long lastCheck2 = 0;
const unsigned long registerUpdateInterval = 2500;
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


/////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////
//INA Functions
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
  if (!ina260_0x45.begin(0x45)) {
    Serial.println("Couldn't find INA260 at 0x45");
    while (1);
  } 
}

///////////////////////////////////////////////////////////////////
void EnableBalancerPins(){
  //Define Pins as outputs to balancer
  //Cell 1 Balance
  pinMode(PB1, OUTPUT);
  //Cell 2 Balance
  pinMode(PB0, OUTPUT);
  //Cell 3 Balance
  pinMode(PA7, OUTPUT);
  //Cell 4 Balance
  pinMode(PA6, OUTPUT);

}
void Balance_Cells(bool bal){

  //Grab each cells Voltage
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage()/1000.00;
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage()/1000.00;
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage()/1000.00;
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage()/1000.00;

  //Calulate Cell Voltages based off of INA readings
  CELL1_VOLTAGE = INA_0x45_VOLTAGE - INA_0x44_VOLTAGE;
  CELL2_VOLTAGE = INA_0x44_VOLTAGE - INA_0x41_VOLTAGE;
  CELL3_VOLTAGE = INA_0x41_VOLTAGE - INA_0x40_VOLTAGE;
  CELL4_VOLTAGE = INA_0x40_VOLTAGE;

  //Grab LTC voltage too
  CELL1_VOLTAGE_LTC2943 = Request_Voltage_LTC2943()- INA_0x44_VOLTAGE;
  
  //Balance if true
  if(bal == true)
  {
    //If cell 1 outside of CellVMax Activate Balancer on Cell 1
    if(CELL1_VOLTAGE_LTC2943 >= CellVMax){
      Serial.print("CELL 1 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PB1, HIGH);
      Serial.print("CELL 1 (LTC) Voltage: ");
      Serial.println(CELL1_VOLTAGE_LTC2943);
      Serial.print("CELL 1 (INA) Voltage: ");
      Serial.println(CELL1_VOLTAGE);
    }
    else{
      Serial.print("Cell 1 in balance\n");
      digitalWrite(PB1, LOW);
      Serial.print("CELL 1 (LTC) Voltage: ");
      Serial.println(CELL1_VOLTAGE_LTC2943);
      Serial.print("CELL 1 (INA) Voltage: ");
      Serial.println(CELL1_VOLTAGE);
    }
    //If cell 2 outside of CellVMax Activate Balancer on Cell 2
    if(CELL2_VOLTAGE >= CellVMax)
    {
      Serial.print("CELL 2 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PB0, HIGH);
      Serial.print("CELL 2 Voltage: ");
      Serial.println(CELL2_VOLTAGE);
    }
    else{
      Serial.print("Cell 2 in balance\n");
      digitalWrite(PB0, LOW);
      Serial.print("CELL 2 Voltage: ");
      Serial.println(CELL2_VOLTAGE);
    }
    //If cell 3 outside of CellVMax Activate Blancer on Cell 3
    if(CELL3_VOLTAGE >= CellVMax)
    {
      Serial.print("CELL 3 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PA7, HIGH);
      Serial.print("CELL 3 Voltage: ");
      Serial.println(CELL3_VOLTAGE);

    }
    else{
      Serial.print("Cell 3 in balance\n");
      digitalWrite(PA7, LOW);
      Serial.print("CELL 3 Voltage: ");
      Serial.println(CELL3_VOLTAGE);
    }
    //If cell 4 outside of CellVMax Activate Blancer on Cell 4
    //USING LTC_2943
    if(CELL4_VOLTAGE >= CellVMax)
    {
      Serial.print("CELL 4 OUT OF VOLTAGE RANGE, BYPASSING\n");
      digitalWrite(PA6, HIGH);
      Serial.print("CELL 4 Voltage: ");
      Serial.println(CELL4_VOLTAGE);
      Serial.println("");
    }
    else{
      Serial.print("Cell 4 in balance\n");
      digitalWrite(PA6, LOW);
      Serial.print("CELL 4 Voltage: ");
      Serial.println(CELL4_VOLTAGE);
      Serial.println("");
    }

  }
  else{
    Serial.print("CELL 1 (LTC) Voltage: ");
    Serial.println(CELL1_VOLTAGE_LTC2943);
    Serial.print("CELL 1 (INA) Voltage: ");
    Serial.println(CELL1_VOLTAGE);
    Serial.print("CELL 2 Voltage: ");
    Serial.println(CELL2_VOLTAGE);
    Serial.print("CELL 3 Voltage: ");
    Serial.println(CELL3_VOLTAGE);
    Serial.print("CELL 4 Voltage: ");
    Serial.println(CELL4_VOLTAGE);
    Serial.println("");

    /**
    Serial.print("Voltage of ina260_0x40 = ");
    Serial.println(INA_0x40_VOLTAGE); 
    Serial.print("Voltage of ina260_0x41 = ");
    Serial.println(INA_0x41_VOLTAGE); 
    Serial.print("Voltage of ina260_0x44 = ");
    Serial.println(INA_0x44_VOLTAGE); 
    Serial.print("Voltage of ina260_0x45 = ");
    Serial.println(INA_0x45_VOLTAGE); 
    Serial.println("");
    **/
  }
 
}
/////////////////////////////////////////////////////////////////////////
void printSerialMenu() {
  Serial.println(F("\n=== Charging Control Menu ==="));
  Serial.println(F("  Type START to begin charging"));
  Serial.println(F("  Type STOP  to halt charging"));
  Serial.println(F("=============================\n"));
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
        enableCharging();
        CHARGING = true;
        Serial.println(F("⚡ Charging ENABLED"));
      }
      else if (cmd == "STOP") {
        disableCharging();
        CHARGING = false;
        Serial.println(F("⛔ Charging DISABLED"));
      }
      else {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        Serial.println(F("  Use START or STOP"));
      }
      serialCmdBuffer = "";             // clear for next line
    } else {
      serialCmdBuffer += c;             // accumulate chars
      // optional: cap buffer length to avoid runaway
      if (serialCmdBuffer.length() > 20)
        serialCmdBuffer = serialCmdBuffer.substring(serialCmdBuffer.length() - 20);
    }
  }
}


/////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  delay(3000);
  Wire.begin();
  delay(100);
  setupLTC2943();
  CheckBQConnectionWithComms();
  CheckIfINAConnected();
  EnableBalancerPins();
  printSerialMenu();
}

void loop() {
  Serial.println("----------------------------------------------------");
  //LTC2943 loop code 
  // Keep analog section active if Pack is disconnected temporarily
  write_LTC2943_Register(REG_CONTROL, 0b11111000);

  //Grrab user Input
  handleSerialCharging();

  //BQ loop code, Charge if User wants Charge Dont charge otherwise
  Balance_Cells(CHARGING);
  if(CHARGING == true){
    MaintainChargingBQ();
  }
  

  //Print out LTC status
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
  
  Serial.println("----------------------------------------------------");
  Serial.println("");
  delay(2000);
}
//EOF