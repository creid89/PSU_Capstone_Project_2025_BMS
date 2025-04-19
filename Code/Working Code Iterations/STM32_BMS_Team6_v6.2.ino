#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>
#include "stm32f1xx.h"  // Include STM32F1 device header for register access

/*
  Working on casting the cell voltage value from a floating point to its raw bytes to send across I2C.
  This version integrates the I2C2 bus. The Serial2 bus for UART is left in.
*/

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

// Create a new instance for the I2C2 bus to be used as a slave
TwoWire myI2C2(PB11, PB10);
#define SLAVE_ADDRESS 0x60

// Declare responseData as a float so we can send a 4-byte float over I2C
float responseData = 0.0;

// Create instances for each INA260 sensor
Adafruit_INA260 ina260_0x40;
Adafruit_INA260 ina260_0x41;
Adafruit_INA260 ina260_0x44;
Adafruit_INA260 ina260_0x45;

// Volatile flags set by the timer ISR
volatile bool serialCheckFlag = false;
volatile bool INA260_0x40_Flag = false;
volatile bool INA260_0x41_Flag = false;
volatile bool INA260_0x44_Flag = false;
volatile bool INA260_0x45_Flag = false;

// Global Variables for sensor readings
float PACK_CURRENT = 0.0;
float INA_0x40_VOLTAGE = 0.0;
float INA_0x41_VOLTAGE = 0.0;
float INA_0x44_VOLTAGE = 0.0;
float INA_0x45_VOLTAGE = 0.0;
float CELL1_VOLTAGE = 0.0;
float CELL2_VOLTAGE = 0.0;
float CELL3_VOLTAGE = 0.0;
float CELL4_VOLTAGE = 0.0;
float TOTAL_PACK_VOLTAGE = 0.0;

// Create a timer instance using TIM2 (adjust if needed)
STM32Timer ITimer2(TIM2);

// Timer ISR: set flags for sensor and serial update
void onTimerISR() {
  serialCheckFlag = true;
  INA260_0x40_Flag = true;
  INA260_0x41_Flag = true;
  INA260_0x44_Flag = true;
  INA260_0x45_Flag = true;
}


#define BQ25730_ADDR 0x6B  // 7-bit I2C address

//BQ25730 Register addresses
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

// BQ25730 Configuration parameters
float ChargeVoltageValue = 17.000;         // Charging target voltage
float VsysMinValue = 13.3;                 // Minimum system voltage
float inputVoltageValue = 12.0;            // Input voltage limit (used for VINDPM)
float chargeCurrentValue = 1.00;           // Battery charging current (in amps)
float inputCurrentLimitValue = 1000.00;    // Input current limit (in milliamps) - based on supply/adapter limit

unsigned long lastCheck1 = 0;
const unsigned long I2CcheckInterval = 5000;

unsigned long lastCheck2 = 0;
const unsigned long registerUpdateInterval = 2500;



void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }
  // Init I2C2 in slave mode at SLAVE_ADDRESS 0x60
  myI2C2.begin(SLAVE_ADDRESS);
  
  // Initialize Serial2 for UART communication
  Serial2.begin(9600);
  
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

  // Start the timer interrupt to trigger every 500,000 microseconds (500 ms)
  if (ITimer2.attachInterruptInterval(500000, onTimerISR)) {
    Serial.println("Timer2 started, checking sensors every 500 ms");
  } else {
    Serial.println("Failed to start Timer2");
  }

  // Register I2C callbacks (register once in setup)
  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);
  
  Serial.println("I2C slave initialized on SDA (PB11) and SCL (PB10)");

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
  // Update sensor readings based on timer flags
  if (INA260_0x40_Flag) {
    INA260_0x40_Flag = false;
    read_INA260_0x40_values();
  }
  if (INA260_0x41_Flag) {
    INA260_0x41_Flag = false;
    read_INA260_0x41_values();
  }
  if (INA260_0x44_Flag) {
    INA260_0x44_Flag = false;
    read_INA260_0x44_values();
  }
  if (INA260_0x45_Flag) {
    INA260_0x45_Flag = false;
    read_INA260_0x45_values();
  }

  // Optionally, re-register I2C callbacks if required by your core
  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);

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

float getAveragePackCurrent() {
  const int sampleCount = 20;
  float currentReadings[sampleCount];
  float totalCurrent = 0.0;

  for (int i = 0; i < sampleCount; i++) {
    // Read current in mA and convert to Amps
    float current = ina260_0x45.readCurrent() / 1000.000;
    currentReadings[i] = current;
    totalCurrent += current;

    delay(30); // Small delay between samples (tweak if needed)
  }

  float averageCurrent = totalCurrent / sampleCount;

  Serial.print("Average Pack Current (A): ");
  Serial.println(averageCurrent);

  return averageCurrent;
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

// -----------------------------------------------
// Charger Configuration Register Setup Functions
// -----------------------------------------------

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
// BQ25730 Debug Utility
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

//--------------------------------------------------
//  INA260 Voltage and Current Monitoring Functions
//--------------------------------------------------

void onReceive(int howMany) {
  // Read one command byte from the master.
  uint16_t receivedCommand = myI2C2.read();

  switch(receivedCommand) {
    case 0x01:
      Serial.println("Requested Voltage of Cell 1");
      // Calculate cell 1 voltage from sensor readings
      CELL1_VOLTAGE = INA_0x45_VOLTAGE - INA_0x44_VOLTAGE;
      Serial.print("Voltage of Cell 1: ");
      Serial.println(CELL1_VOLTAGE);
      // Set the floating point response
      responseData = CELL1_VOLTAGE;
      break;
      
    case 0x02:
      Serial.println("Requested Voltage of Cell 2");
      CELL2_VOLTAGE = INA_0x44_VOLTAGE - INA_0x41_VOLTAGE;
      Serial.print("Voltage of Cell 2: ");
      Serial.println(CELL2_VOLTAGE);
      responseData = CELL2_VOLTAGE;
      break;
      
    case 0x03:
      Serial.println("Requested Voltage of Cell 3");
      CELL3_VOLTAGE = INA_0x41_VOLTAGE - INA_0x40_VOLTAGE;
      Serial.print("Voltage of Cell 3: ");
      Serial.println(CELL3_VOLTAGE);
      responseData = CELL3_VOLTAGE;
      break;
      
    case 0x04:
      Serial.println("Requested Voltage of Cell 4");
      CELL4_VOLTAGE = INA_0x40_VOLTAGE;
      Serial.print("Voltage of Cell 4: ");
      Serial.println(CELL4_VOLTAGE);
      responseData = CELL4_VOLTAGE;
      break;
      
    case 0x05:
      Serial.println("Requested Total Pack Voltage");
      TOTAL_PACK_VOLTAGE = ina260_0x45.readBusVoltage() / 1000.0;
      Serial.print("Total Pack Voltage: ");
      Serial.println(TOTAL_PACK_VOLTAGE);
      responseData = TOTAL_PACK_VOLTAGE;
      break;
      
    case 0x06:
      Serial.println("Requested Current Going Through Pack");
      PACK_CURRENT = getAveragePackCurrent() / 1000.0; // read current in A
      Serial.print("Current Through the Pack: ");
      Serial.println(PACK_CURRENT);
      responseData = PACK_CURRENT;
      break;
      
    case 0x07:
      Serial.println("Requested Power Consumed");
      // Add code to compute power consumed and update responseData.
      responseData = 0.0;
      break;
      
    case 0x08:
      Serial.println("Requested State of Charge");
      // Add code to compute state of charge and update responseData.
      responseData = 0.0;
      break;
      
    default:
      Serial.println("Unknown Command Received");
      responseData = -1.0;  // Use -1.0 as an error indicator
  }
}

void onRequest() {
  // Send the 4-byte float responseData over I2C.
  myI2C2.write((uint8_t *)&responseData, sizeof(responseData));
  // Optionally, reset responseData after sending.
  responseData = 0.0;
}

void read_INA260_0x40_values() {
  delay(1000);
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage() / 1000.0;
  Serial.print("\nVoltage of ina260_0x40 = ");
  Serial.println(INA_0x40_VOLTAGE); 
}

void read_INA260_0x41_values() {
  delay(1000);
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage() / 1000.0;
  Serial.print("Voltage of ina260_0x41 = ");
  Serial.println(INA_0x41_VOLTAGE); 
}

void read_INA260_0x44_values() {
  delay(1000);
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage() / 1000.0;
  Serial.print("Voltage of ina260_0x44 = ");
  Serial.println(INA_0x44_VOLTAGE);
}

void read_INA260_0x45_values() {
  delay(1000);
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage() / 1000.0;
  Serial.print("Voltage of ina260_0x45 = ");
  Serial.println(INA_0x45_VOLTAGE);
  delay(1000);  
  PACK_CURRENT = getAveragePackCurrent(); // read current in A
  Serial.print("Current Through the Pack = ");
  Serial.println(PACK_CURRENT);
  Serial.println("\n--------------------------------------\n");
}