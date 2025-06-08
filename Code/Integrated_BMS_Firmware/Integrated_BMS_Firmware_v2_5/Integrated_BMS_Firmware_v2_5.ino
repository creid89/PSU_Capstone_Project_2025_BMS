//Integrated BMS Firmware V2_3 //Sodium & Lithium ION BMS //Include wire for I2C #include <Wire.h> //Includes for INA #include <Adafruit_INA260.h> //include for interrupts #include <HardwareSerial.h> #include <STM32TimerInterrupt.h> #include "stm32f1xx.h" #include <EEPROM.h> //EEPROM


/**
 * @file BMS_Firmware_V2_3.ino
 * @brief Integrated Battery Management System Firmware Version 2.3
 * @details Supports both Sodium and Lithium Ion battery chemistries with comprehensive monitoring and protection
 * @version 2.3
 * @date 2024
 * @author BMS Development Team
 */

//! Integrated BMS Firmware V2_3
//! Sodium & Lithium ION BMS

//! Include Wire library for I2C communication
#include <Wire.h>
//! Include Adafruit INA260 current/voltage sensor library
#include <Adafruit_INA260.h>
//! Include hardware serial library for interrupt handling
#include <HardwareSerial.h>
//! Include STM32 timer interrupt library for periodic tasks
#include <STM32TimerInterrupt.h>
//! Include STM32 hardware abstraction layer
#include "stm32f1xx.h"
//! Include EEPROM library for persistent storage
#include <EEPROM.h>

/**
 * @defgroup EEPROM_Storage EEPROM Storage Addresses
 * @brief EEPROM storage locations for system initialization values passed by controller MCU
 * @{
 */
#define EEPROM_ADDR_1  0                                       //!< EEPROM address for float value 1 (ChargeVoltageValue)
#define EEPROM_ADDR_2  (EEPROM_ADDR_1 + (2*sizeof(float)))     //!< EEPROM address for float value 2 (VsysMinValue)
#define EEPROM_ADDR_3  (EEPROM_ADDR_2 + (2*sizeof(float)))     //!< EEPROM address for float value 3 (inputVoltageValue)
#define EEPROM_ADDR_4  (EEPROM_ADDR_3 + (2*sizeof(float)))     //!< EEPROM address for float value 4 (chargeCurrentValue)
#define EEPROM_ADDR_5  (EEPROM_ADDR_4 + (2*sizeof(float)))     //!< EEPROM address for float value 5 (inputCurrentLimitValue)
#define EEPROM_ADDR_6  (EEPROM_ADDR_5 + (2*sizeof(float)))     //!< EEPROM address for float value 6 (CellMaxCutOffV)
#define EEPROM_ADDR_7  (EEPROM_ADDR_6 + (2*sizeof(float)))     //!< EEPROM address for float value 7 (CellMinCutOffV)
#define EEPROM_ADDR_8  (EEPROM_ADDR_7 + (2*sizeof(float)))     //!< EEPROM address for float value 8 (Pack_stock_capacity)
#define EEPROM_ADDR_9  (EEPROM_ADDR_8 + (2*sizeof(float)))     //!< EEPROM address for float value 9 (EEPROM_CONFIG_FLAG)
#define EEPROM_TOTAL_LENGTH (9 * (2*sizeof(float)))            //!< Total EEPROM length for 9 float values
/** @} */

////////////////////////////////////////////////////////////////
/**
 * @defgroup INA260_Instances INA260 Sensor Instances
 * @brief Create instances for each INA260 current/voltage sensor
 * @{
 */
Adafruit_INA260 ina260_0x40; //!< INA260 sensor instance at I2C address 0x40
Adafruit_INA260 ina260_0x41; //!< INA260 sensor instance at I2C address 0x41
Adafruit_INA260 ina260_0x44; //!< INA260 sensor instance at I2C address 0x44
/** @} */

/**
 * @defgroup System_Flags Major System Control Flags
 * @brief Global flags controlling major system operations
 * @{
 */
volatile bool CHARGING = false;             //!< Flag indicating if charging is currently active
String serialCmdBuffer = "";               //!< Buffer for accumulating incoming serial characters for charge control
volatile bool CHARGE_ON_PLUGIN = true;     //!< When true, charging auto-starts on charger plugin detection
volatile bool STOPCHARGING = false;        //!< Flag to request charging stop
volatile bool KILLLOAD = false;            //!< Flag to disconnect load
volatile bool PLUGGEDIN = false;           //!< Flag indicating charger is physically connected
volatile bool OVERHEAT = false;            //!< Flag indicating thermal protection activation
volatile bool ERRORFLG = false;            //!< General error flag for system faults
/** @} */

/**
 * @defgroup Timer_Flags Timer and Configuration Flags
 * @brief Flags related to timing and system configuration
 * @{
 */
volatile bool EEPROM_LOADED = false;       //!< Flag indicating EEPROM data has been loaded
float EEPROM_CONFIG_FLAG;                  //!< Configuration flag stored in EEPROM
volatile bool CONFIGURED = false;          //!< Flag indicating system is fully configured
/** @} */

int configstatus = 0;                      //!< Configuration status counter (0-8)
int ConfigStatusFromMaster;                //!< Configuration status received from master controller

//! Timer instance using TIM2 for periodic system checks
STM32Timer ITimer2(TIM2);

/**
 * @brief Timer interrupt service routine
 * @details Called periodically to set flags for sensor and serial updates
 * @note Currently contains only a test placeholder
 */
void onTimerISR() {
  //TEST_FLAG = true;  //!< Placeholder for future timer-based operations
}

/**
 * @defgroup INA_BQ_Variables INA260 and BQ25730 Variables
 * @brief Variables for storing sensor readings and measurements
 * @{
 */
float INA_0x40_VOLTAGE = 0.0;              //!< Voltage reading from INA260 at address 0x40
float INA_0x41_VOLTAGE = 0.0;              //!< Voltage reading from INA260 at address 0x41
float INA_0x44_VOLTAGE = 0.0;              //!< Voltage reading from INA260 at address 0x44
float CELL1_VOLTAGE_LTC2943 = 0.00;        //!< Cell 1 voltage calculated from LTC2943 readings
float CELL1_VOLTAGE = 0.00;                //!< Cell 1 voltage (alternative calculation)
float CELL2_VOLTAGE = 0.00;                //!< Cell 2 voltage calculated from INA differential readings
float CELL3_VOLTAGE = 0.00;                //!< Cell 3 voltage calculated from INA differential readings
float CELL4_VOLTAGE = 0.00;                //!< Cell 4 voltage (bottom cell in series stack)
/** @} */

//! Thermistor threshold value: 3.3V / 2 = 1.65V scaled to 12-bit ADC: (1.65 / 3.3) * 4095 ≈ 2047
const int THERMISTER_THRESHOLD = 2047;

/**
 * @defgroup LTC2943_Constants LTC2943 Scaling Factors and Constants
 * @brief Constants and scaling factors for LTC2943 fuel gauge calculations
 * @{
 */
float resistor = .1;                                    //!< Current sense resistor value in ohms for LTC2943
const float LTC2943_CHARGE_lsb = 0.34E-3;             //!< Charge LSB scaling factor
const float LTC2943_VOLTAGE_lsb = 1.44E-3;            //!< Voltage LSB scaling factor
const float LTC2943_CURRENT_lsb = 29.3E-6;            //!< Current LSB scaling factor
const float LTC2943_TEMPERATURE_lsb = 0.25;           //!< Temperature LSB scaling factor
const float LTC2943_FULLSCALE_VOLTAGE = 23.6;         //!< Full scale voltage range
const float LTC2943_FULLSCALE_CURRENT = 60E-3;        //!< Full scale current range
const float LTC2943_FULLSCALE_TEMPERATURE = 510;      //!< Full scale temperature range
float prescaler = 4096;                                //!< Prescaler value for calculations
/** @} */

/**
 * @defgroup Balancer_Variables Cell Balancing Variables
 * @brief Variables controlling cell balancing thresholds
 * @{
 */
float CellMaxCutOffV;                      //!< Maximum cell voltage cutoff (user configurable)
float CellFullChargeV = CellMaxCutOffV - 0.1; //!< Full charge voltage threshold (0.1V below max)
float CellMinCutOffV;                      //!< Minimum cell voltage cutoff (user configurable)
/** @} */

/**
 * @defgroup System_Returns System Return Variables
 * @brief Variables returned to main controller system
 * @{
 */
float PackVoltage = 0;                     //!< Total battery pack voltage
float current = 0;                         //!< Pack current (positive = charging, negative = discharging)
float tempC = 0;                          //!< Battery temperature in Celsius
float charge_mAh = 0;                     //!< Raw accumulated charge from LTC2943
float real_charge_mAh = 0;                //!< Adjusted accumulated charge accounting for capacity offset
/** @} */

/**
 * @defgroup LTC2943_Config LTC2943 Configuration Parameters
 * @brief Configuration parameters for LTC2943 fuel gauge
 * @{
 */
float Pack_stock_capacity = 5200;          //!< Battery pack stock capacity in mAh (user configurable)
const float RSENSE = 0.1f;                //!< Current sense resistor value in ohms
const float LTC2943_PRESCALER = 16384.0f; //!< Control register prescaler setting (bits B[5:3] = 0b111 → M = 16384)
/** @} */

//! Charge LSB calculation: qLSB = 0.340 mAh × (50 mΩ / RSENSE) × (M / 4096) per datasheet p.15
const float QLSB = 0.340f * (50e-3f / RSENSE) * (LTC2943_PRESCALER / 4096.0f);

int numOfCells = 4;                        //!< Number of cells in series (4S configuration)

/**
 * @defgroup BQ25730_Config BQ25730 Charger Configuration Parameters
 * @brief Configuration parameters for BQ25730 battery charger
 * @{
 */
float ChargeVoltageValue=18;               //!< Charging target voltage - will be updated to (numOfCells * CellMaxCutOffV) + 1
float VsysMinValue;                        //!< Minimum system voltage threshold
float inputVoltageValue;                   //!< Input voltage limit for VINDPM (input voltage DPM)
float chargeCurrentValue=1;                //!< Battery charging current in amps
float inputCurrentLimitValue = 1;          //!< Input current limit in amps (based on supply/adapter limit)
/** @} */

/**
 * @defgroup Timing_Variables Timing Control Variables
 * @brief Variables for controlling periodic operations timing
 * @{
 */
unsigned long lastCheck1 = 0;              //!< Timestamp for last I2C communication check
const unsigned long I2CcheckInterval = 5000; //!< I2C check interval in milliseconds
unsigned long lastCheck2 = 0;              //!< Timestamp for last register update
const unsigned long registerUpdateInterval = 2500; //!< Register update interval in milliseconds
/** @} */

//////////////////////////////////////////////////////////////////////

/**
 * @defgroup I2C_Communication I2C Communication Setup
 * @brief I2C communication configuration for controller interface
 * @{
 */
float responseData = 0.0;                  //!< Data to be sent back to I2C master
TwoWire myI2C2(PB11, PB10);               //!< I2C interface on pins PB11 (SDA) and PB10 (SCL)
#define SLAVE_ADDRESS 0x58                 //!< I2C slave address for STM32 controller communications
/** @} */

/**
 * @defgroup Device_Addresses I2C Device Addresses
 * @brief I2C addresses for external devices
 * @{
 */
#define LTC2943_ADDR 0x64                  //!< 7-bit I2C address for LTC2943 fuel gauge
#define BQ25730_ADDR 0x6B                  //!< 7-bit I2C address for BQ25730 battery charger
/** @} */

/**
 * @defgroup LTC2943_Registers LTC2943 Register Addresses
 * @brief Register addresses for LTC2943 fuel gauge communication
 * @{
 */
#define REG_STATUS     0x00                //!< Status register
#define REG_CONTROL    0x01                //!< Control register
#define REG_ACC_CHARGE_MSB 0x02           //!< Accumulated charge MSB register
#define REG_VOLTAGE_MSB    0x08           //!< Voltage measurement MSB register
#define REG_CURRENT_MSB    0x0E           //!< Current measurement MSB register
#define REG_TEMPERATURE_MSB 0x14          //!< Temperature measurement MSB register
/** @} */

/**
 * @defgroup BQ25730_Registers BQ25730 Register Addresses
 * @brief Register addresses for BQ25730 battery charger communication
 * @{
 */
#define ChargeOption0   0x00               //!< Charge option 0 register
#define ChargeCurrent   0x02               //!< Charge current setting register
#define ChargeVoltage   0x04               //!< Charge voltage setting register
#define OTGVoltage      0x06               //!< OTG voltage setting register
#define OTGCurrent      0x08               //!< OTG current setting register
#define InputVoltage    0x0A               //!< Input voltage limit register
#define VSYS_MIN        0x0C               //!< Minimum system voltage register
#define IIN_HOST        0x0E               //!< Input current limit register
#define ChargerStatus   0x20               //!< Charger status register
#define ProchotStatus   0x22               //!< PROCHOT status register
#define IIN_DPM         0x24               //!< Input current DPM register
#define AADCVBUS_PSYS   0x26               //!< ADC VBUS and PSYS register
#define ADCIBAT         0x28               //!< ADC battery current register
#define ADCIINCMPIN     0x2A               //!< ADC input current and CMPIN register
#define ADCVSYSVBAT     0x2C               //!< ADC VSYS and VBAT register
#define MANUFACTURER_ID 0x2E               //!< Manufacturer ID register
#define DEVICE_ID       0x2F               //!< Device ID register
#define ChargeOption1   0x30               //!< Charge option 1 register
#define ChargeOption2   0x32               //!< Charge option 2 register
#define ChargeOption3   0x34               //!< Charge option 3 register
#define ProchotOption0  0x36               //!< PROCHOT option 0 register
#define ProchotOption1  0x38               //!< PROCHOT option 1 register
#define ADCOption       0x3A               //!< ADC option register
#define ChargeOption4   0x3C               //!< Charge option 4 register
#define Vmin_Active_Protection 0x3E        //!< Minimum voltage active protection register
/** @} */

//////////////////////////////////////////////////////////////////////

/**
 * @brief Perform software reset of the microcontroller
 * @details Triggers a system reset using NVIC (Nested Vector Interrupt Controller)
 */
void softwareReset() {
  NVIC_SystemReset(); //!< Execute system reset
}

/**
 * @defgroup LTC2943_Functions LTC2943 Fuel Gauge Functions
 * @brief Functions for communicating with and reading from LTC2943 fuel gauge
 * @{
 */

/**
 * @brief Write data to LTC2943 register over I2C
 * @param reg Register address to write to
 * @param value 8-bit value to write to the register
 */
void write_LTC2943_Register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LTC2943_ADDR); //!< Start I2C transmission to LTC2943
  Wire.write(reg);                      //!< Send register address
  Wire.write(value);                    //!< Send data value
  Wire.endTransmission();               //!< End I2C transmission
}

/**
 * @brief Read 16-bit data from LTC2943 register over I2C
 * @param reg Register address to read from
 * @return 16-bit value read from the register, or 0 if read failed
 */
uint16_t Read_LTC2943_Register(uint8_t reg) {
  Wire.beginTransmission(LTC2943_ADDR); //!< Start I2C transmission to LTC2943
  Wire.write(reg);                      //!< Send register address
  Wire.endTransmission(false);          //!< End transmission but keep bus active (repeated start)
  Wire.requestFrom(LTC2943_ADDR, (uint8_t)2); //!< Request 2 bytes from LTC2943
  if (Wire.available() < 2) {           //!< Check if sufficient data is available
    Serial.println("I2C read failed");  //!< Print error message
    return 0;                          //!< Return 0 on failure
  }
  uint16_t value = Wire.read() << 8;    //!< Read MSB and shift left 8 bits
  value |= Wire.read();                 //!< Read LSB and combine with MSB
  return value;                         //!< Return combined 16-bit value
}

/**
 * @brief Request voltage measurement from LTC2943
 * @return Battery pack voltage in volts
 */
float Request_Voltage_LTC2943(){
  uint16_t vRaw = Read_LTC2943_Register(REG_VOLTAGE_MSB); //!< Read raw voltage register
  float LTCvoltage = 23.6 * vRaw / 65535.0; //!< Convert to voltage using 16-bit, 23.6V full-scale
  return LTCvoltage;                    //!< Return calculated voltage
}

/**
 * @brief Request current measurement from LTC2943
 * @return Battery pack current in milliamps (positive = charging, negative = discharging)
 */
float Request_Current_LTC2943(){
  uint16_t iRaw = Read_LTC2943_Register(REG_CURRENT_MSB); //!< Read raw current register
  int16_t iOffset = iRaw - 32767;      //!< Convert from offset binary (12-bit, ±60mV full scale)
  float LTCcurrent = (60.0 / RSENSE) * iOffset / 32767.0; //!< Convert to current using sense resistor
  return LTCcurrent;                    //!< Return calculated current
}

/**
 * @brief Request temperature measurement from LTC2943
 * @return Battery temperature in degrees Celsius
 */
float Request_Temp_LTC2943(){
    uint16_t tRaw = Read_LTC2943_Register(REG_TEMPERATURE_MSB); //!< Read raw temperature register
  float tempK = 510.0 * tRaw / 65535.0; //!< Convert to Kelvin using full-scale range
  float tempC = tempK - 273.15;         //!< Convert from Kelvin to Celsius
  return tempC;                         //!< Return temperature in Celsius
}

/**
 * @brief Request State of Charge (SoC) from LTC2943
 * @return Accumulated charge in milliamp-hours
 */
float Request_SoC_LTC2943(){
  uint16_t acr = Read_LTC2943_Register(REG_ACC_CHARGE_MSB); //!< Read accumulated charge register
  charge_mAh = (acr - 32767) * QLSB;   //!< Convert using charge LSB and offset binary format
  return charge_mAh;                    //!< Return calculated charge
}

/**
 * @brief Initialize and configure LTC2943 fuel gauge
 * @details Resets charge accumulator and configures for automatic operation
 */
void setupLTC2943() {
  write_LTC2943_Register(REG_CONTROL, 0b11111001); //!< Reset charge accumulator (bit 0 = 1)
  write_LTC2943_Register(REG_CONTROL, 0b11111000); //!< Re-enable analog section, auto mode, prescaler 4096
}

/** @} */

//////////////////////////////////////////////////////////////////////

/**
 * @defgroup BQ25730_Functions BQ25730 Battery Charger Functions
 * @brief Functions for controlling and monitoring BQ25730 battery charger
 * @{
 */

/**
 * @brief Check BQ25730 connection and halt system if not responding
 * @details Verifies I2C communication with BQ25730 and stops execution if failed
 */
void CheckBQConnectionWithComms(){
if (!checkBQConnection()) {             //!< Test I2C connection to BQ25730
    Serial.println("BQ25730 not responding on I2C. Check wiring or power."); //!< Print error message
    while (1); //!< Halt system execution indefinitely
  } else {
    Serial.println("BQ25730 connection verified."); //!< Confirm successful connection
  }
}

/**
 * @brief Configure and start BQ25730 charger with initial settings
 * @details Sets charge voltage, current limits, enables ADC, and starts charging
 */
void ConfigAndStartBQ(){
  setChargeVoltage(ChargeVoltageValue);     //!< Set target charging voltage
  setInputVoltage(inputVoltageValue);       //!< Set input voltage limit
  setChargeCurrent(chargeCurrentValue);     //!< Set charging current
  setInputCurrentLimit(inputCurrentLimitValue); //!< Set input current limit
  set_VSYS_MIN(VsysMinValue);              //!< Set minimum system voltage
  enableADC();                             //!< Enable ADC measurements
  enableCharging();                        //!< Enable charging operation
}

/**
 * @brief Write 16-bit data to BQ25730 register
 * @param regAddr Register address to write to
 * @param data 16-bit data to write (LSB first, then MSB)
 */
void writeBQ25730(uint8_t regAddr, uint16_t data) {
  Wire.beginTransmission(BQ25730_ADDR);    //!< Start I2C transmission to BQ25730
  Wire.write(regAddr);                     //!< Send register address
  Wire.write(data & 0xFF);                 //!< Send low byte
  Wire.write((data >> 8) & 0xFF);          //!< Send high byte
  Wire.endTransmission();                  //!< End I2C transmission
}

/**
 * @brief Read 16-bit data from BQ25730 register
 * @param regAddr Register address to read from
 * @return 16-bit data read from register
 */
uint16_t readBQ25730(uint8_t regAddr) {
  Wire.beginTransmission(BQ25730_ADDR);    //!< Start I2C transmission to BQ25730
  Wire.write(regAddr);                     //!< Send register address
  Wire.endTransmission(false);             //!< End transmission with repeated start
  Wire.requestFrom(BQ25730_ADDR, 2);       //!< Request 2 bytes from BQ25730
  uint16_t data = 0;                       //!< Initialize data variable
  if (Wire.available() == 2) {             //!< Check if 2 bytes are available
    data = Wire.read();                    //!< Read low byte
    data |= Wire.read() << 8;              //!< Read high byte and combine
  }
  return data;                             //!< Return combined 16-bit data
}

/**
 * @brief Check I2C communication with BQ25730
 * @return true if device responds with valid ID, false otherwise
 */
bool checkI2C() {
  uint16_t id = readBQ25730(DEVICE_ID);    //!< Read device ID register
  return (id != 0x0000 && id != 0xFFFF);  //!< Return true if ID is valid (not 0x0000 or 0xFFFF)
}

/**
 * @brief Check basic I2C connection to BQ25730
 * @return true if I2C transmission successful, false otherwise
 */
bool checkBQConnection() {
  Wire.beginTransmission(BQ25730_ADDR);    //!< Start I2C transmission to BQ25730
  return (Wire.endTransmission() == 0);    //!< Return true if transmission successful (no error)
}

/**
 * @brief Disable VSYS output due to low battery condition
 * @details Forces battery FET off to disconnect system load when battery is critically low
 */
void disableVSYS() {
  Serial.println("------------- VSYS HAS BEEN DISABLED BECAUSE OF LOW BATTERY -----------------"); //!< Print warning message

    uint16_t chargeOption0 = readBQ25730(0x12); //!< Read current ChargeOption0 register
    chargeOption0 &= ~(1 << 5);             //!< Clear EN_OTG bit (disable OTG mode)
    writeBQ25730(0x12, chargeOption0);      //!< Write back modified register

    uint16_t chargeOption3 = readBQ25730(0x34); //!< Read current ChargeOption3 register
    chargeOption3 &= ~(1 << 1);             //!< Clear BATFETOFF_HIZ bit
    writeBQ25730(0x34, chargeOption3);      //!< Write back modified register

    chargeOption3 |= (1 << 7);              //!< Set BATFET_ENZ bit to force battery FET off
    writeBQ25730(0x34, chargeOption3);      //!< Write back modified register

    chargeOption3 = readBQ25730(0x34);      //!< Read back register for confirmation
    Serial.print("ChargeOption3 after BATFET_ENZ set: "); //!< Print confirmation message
    Serial.println(chargeOption3, HEX);     //!< Print register value in hex
}

/**
 * @brief Set charging voltage target
 * @param voltage_V Target charging voltage in volts (1.024V - 23.000V range)
 */
void setChargeVoltage(float voltage_V) {
  if (voltage_V < 1.024 || voltage_V > 23.000) { //!< Check if voltage is within valid range
    Serial.println("Charge voltage out of range (1.024V – 23.000V)."); //!< Print error message
    return;                                //!< Exit function if out of range
  }

  uint16_t stepCount = static_cast<uint16_t>(voltage_V / 0.008 + 0.5); //!< Convert voltage to step count (8mV per step)
  uint16_t encoded = (stepCount & 0x1FFF) << 3; //!< Encode for register format (13 bits shifted left 3)

  Serial.print("Writing 0x"); Serial.print(encoded, HEX); //!< Print encoded value being written
  Serial.print(" to ChargeVoltage ("); Serial.print(voltage_V); Serial.println(" V)"); //!< Print voltage value
  writeBQ25730(ChargeVoltage, encoded);    //!< Write encoded value to charge voltage register
}

/**
 * @brief Set charging current limit
 * @param current_A Charging current in amps (0 - 16.256A range)
 */
void setChargeCurrent(float current_A) {
  if (current_A < 0.0 || current_A > 16.256) { //!< Check if current is within valid range
    Serial.println("Charge current out of range (0 – 16.256 A)."); //!< Print error message
    return;                                //!< Exit function if out of range
  }

  uint16_t stepCount = static_cast<uint16_t>(current_A / 0.128 + 0.5); //!< Convert current to step count (128mA per step)
  uint16_t encoded = (stepCount << 6) & 0x1FC0; //!< Encode for register format (shifted left 6 bits)

  Serial.print("Writing 0x"); Serial.print(encoded, HEX); //!< Print encoded value being written
  Serial.print(" to ChargeCurrent Register ("); Serial.print(current_A); Serial.println(" A)"); //!< Print current value
  writeBQ25730(ChargeCurrent, encoded);    //!< Write encoded value to charge current register
  Serial.println("");                      //!< Print blank line for formatting
}

/**
 * @brief Set input current limit
 * @param current_mA Input current limit in milliamps (50mA - 6350mA range)
 */
void setInputCurrentLimit(float current_mA) {
  if (current_mA < 50.0f) current_mA = 50.0f;   //!< Clamp minimum to 50mA
  if (current_mA > 6350.0f) current_mA = 6350.0f; //!< Clamp maximum to 6350mA

  uint16_t code = static_cast<uint16_t>(current_mA / 50.0f); //!< Convert to code (50mA per step)
  uint16_t regValue = (code << 8) & 0x7F00;    //!< Encode for register format (shifted left 8 bits)

  Serial.print("Writing 0x"); Serial.print(regValue, HEX); //!< Print encoded value being written
  Serial.print(" to IIN_HOST Register ("); Serial.print(current_mA); Serial.println(" mA)"); //!< Print current value
  writeBQ25730(IIN_HOST, regValue);        //!< Write encoded value to input current register
}

/**
 * @brief Sets the input voltage limit for the BQ25730 charge controller
 * 
 * This function configures the minimum input voltage threshold. The charger will not
 * operate if the input voltage falls below this setting.
 * 
 * @param voltage_V Input voltage in volts (valid range: 3.2V - 23.0V)
 * @note The voltage is encoded with 64mV resolution and written to the InputVoltage register
 * @warning Function returns early if voltage is out of range
 */
void setInputVoltage(float voltage_V) {
  if (voltage_V < 3.2 || voltage_V > 23.0) {
    Serial.println("Input voltage out of range (3.2V – 23.0V).");
    return;
  }

  uint16_t stepCount = static_cast<uint16_t>((voltage_V - 3.2) / 0.064 + 0.5);
  uint16_t encoded = (stepCount << 6) & 0x3FC0;

  //delay(50);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to InputVoltage Register ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(InputVoltage, encoded);
}

/**
 * @brief Sets the minimum system voltage (VSYS_MIN) for the BQ25730
 * 
 * This function configures the minimum voltage that the system rail will be regulated to.
 * The system will prioritize maintaining this voltage over charging the battery.
 * 
 * @param voltage_V Minimum system voltage in volts (valid range: 1.0V - 23.0V)
 * @note Voltage is encoded with 0.1V resolution in the upper 8 bits
 * @warning Function returns early if voltage is out of range
 */
void set_VSYS_MIN(float voltage_V) {
  if (voltage_V < 1.0 || voltage_V > 23.0) {
    Serial.println("VSYS_MIN voltage out of range (1.0V – 23.0V).");
    return;
  }

  uint16_t encoded = static_cast<uint16_t>(voltage_V * 10) << 8;

  //delay(50);
  Serial.print("Writing 0x"); Serial.print(encoded, HEX);
  Serial.print(" to VSYS_MIN ("); Serial.print(voltage_V); Serial.println(" V)");
  writeBQ25730(VSYS_MIN, encoded);
}

/**
 * @brief Enables charging by configuring ChargeOption0 register
 * 
 * Disables the watchdog timer and permanently enables charging by writing
 * the appropriate configuration to the ChargeOption0 register.
 * 
 * @note Writes 0xE70E to disable watchdog and enable charging
 */
void enableCharging() {
  // 0xE70E: disables watchdog and enables charging permanently
  writeBQ25730(ChargeOption0, 0xE70E);
}

/**
 * @brief Disables charging by setting charge inhibit bit
 * 
 * Prevents the battery from charging by writing to the ChargeOption0 register.
 * This is used for safety shutdowns and normal charge termination.
 * 
 * @note Writes 0xE70F to inhibit charging while keeping other settings
 */
void disableCharging() {
  uint16_t option0 = readBQ25730(ChargeOption0);
  //option0 &= ~(1 << 0); // Clear CHRG_INHIBIT bit
  writeBQ25730(ChargeOption0, 0xE70F);
}

/**
 * @brief Enables ADC measurements on the BQ25730
 * 
 * Configures the ADC to continuously measure voltages and currents.
 * This is required for monitoring battery and system parameters.
 * 
 * @note Writes 0x40FF to enable all ADC channels
 */
void enableADC() {
  writeBQ25730(ADCOption, 0x40FF);
  //delay(50);
}

// -----------------------------
// Data Readouts
// -----------------------------

/**
 * @brief Reads and displays battery current measurements
 * 
 * Reads the ADCIBAT register to get both charge and discharge current measurements.
 * The register contains separate fields for charge and discharge currents with
 * different LSB values.
 * 
 * @note Discharge current: bits[0:6] at 512 mA/LSB
 * @note Charge current: bits[8:14] at 128 mA/LSB
 * @post Prints current measurements to Serial
 */
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

/**
 * @brief Dumps all BQ25730 registers for debugging
 * 
 * Reads and displays all registers from the BQ25730 charge controller.
 * This is useful for debugging configuration issues and monitoring chip state.
 * 
 * @note Reads registers from 0x00 to 0x3E in 2-byte increments
 * @post Prints all register values in hexadecimal format to Serial
 */
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

/**
 * @brief Maintains charging operation and I2C communication
 * 
 * This function performs periodic maintenance tasks to ensure reliable operation:
 * - Checks I2C communication integrity
 * - Re-initializes I2C bus if communication fails
 * - Periodically updates charge voltage and current registers
 * 
 * @note Should be called regularly during charging to prevent watchdog timeouts
 * @note Uses global variables: lastCheck1, lastCheck2, I2CcheckInterval, registerUpdateInterval
 */
void MaintainChargingBQ(){
  // Periodically check I2C communication
  if (millis() - lastCheck1 > I2CcheckInterval) {
    lastCheck1 = millis();
    if (!checkI2C()) {
      Serial.println("I2C comms lost. Attempting to reinitialize...");
      Wire.end();
      //delay(50);
      Wire.begin();
      //delay(50);
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
/**
 * @brief Initializes and verifies INA260 current sensors
 * 
 * Attempts to initialize three INA260 sensors at different I2C addresses.
 * The system halts if any sensor fails to initialize, as all sensors are
 * required for proper cell voltage monitoring.
 * 
 * @note INA260 sensors at addresses: 0x40, 0x41, 0x44
 * @note Address 0x45 is commented out (optional fourth sensor)
 * @warning System halts in infinite loop if any sensor initialization fails
 */
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

/**
 * @brief Configures GPIO pins for various system functions
 * 
 * Sets up GPIO pins for:
 * - Cell balancing circuits (4 pins for bypass resistors)
 * - Charge status indication (CHG_OK input)
 * - Status LEDs (power, charge, error, status)
 * - Temperature sensors (4 thermistor inputs)
 * 
 * @note Balancer pins: PB1, PB0, PA7, PA6 (cells 1-4)
 * @note LED pins: PB15, PB14, PB13, PB12 (power, charge, error, status)
 * @note Thermistor pins: PA2, PA3, PA4, PA5 (sensors 1-4)
 */
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

/**
 * @brief Monitors cell voltages and state of charge for safety cutoffs
 * 
 * This critical safety function performs multiple checks:
 * - Overvoltage protection: stops charging if any cell exceeds CellMaxCutOffV
 * - Undervoltage protection: stops discharging if any cell falls below CellMinCutOffV  
 * - Full charge detection: stops charging when all cells reach CellFullChargeV
 * - State of charge limits: manages charging based on pack capacity
 * 
 * @note Uses global variables for cell voltages and thresholds
 * @note Updates global CHARGING flag based on safety conditions
 * @post Prints detailed status messages for any triggered conditions
 */
void checkCellandSocCutoff() { 
  //Serial.print("\n\n\nMade it into voltage calcs");
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

/**
 * @brief Main charge and cell balancing control function
 * 
 * This comprehensive function manages the entire charging process:
 * - Checks charge enable/disable conditions based on plugin status
 * - Reads all cell voltages using INA260 sensors and LTC2943
 * - Performs safety checks via checkCellandSocCutoff()
 * - Controls individual cell balancing circuits during charging
 * - Updates and displays all system measurements
 * 
 * The cell balancing logic activates bypass resistors for any cell that
 * exceeds the CellFullChargeV threshold during charging.
 * 
 * @note Cell voltages calculated as differential measurements between INA sensors
 * @note CELL1 = Pack_Voltage - INA_0x44, CELL2 = INA_0x44 - INA_0x41, etc.
 * @post Updates global variables for all measurements and charging state
 * @post Prints comprehensive status information to Serial
 */
void ChargeAndBalanceControl(){
  if (CHARGING && !CHARGE_ON_PLUGIN) {
    Serial.println(F("⚠️  Auto Charge off, ENDING CHARGING"));
      // bail out of charging logic but still report voltages
      CHARGING = false;
      disableCharging();
  }
  else{
    Serial.println("keeping on chargin'");
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
  //Serial.print("\n\n\nMade it through initial charge control logic\n\n\n");
  //Current (12-bit, ±60mV full scale, offset binary)
  current = Request_Current_LTC2943();
  //Serial.print("\n----------------- LINE 650 -------------\n"); 
  disableCharging();
  //delay(50);
  //Serial.print("\n----------------- LINE 653 -------------\n"); 
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
  
  //Serial.print("\n\n\nMade it through charge control logic\n\n\n");
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

/**
 * @brief Processes serial commands for manual charge control
 * 
 * Handles text commands received via Serial interface:
 * - "START": Manually enables charging
 * - "STOP": Manually disables charging  
 * - "AUTO": Toggles automatic charge-on-plugin mode
 * - "RST"/"reset"/"RESET": Triggers software reset
 * 
 * Commands are processed line-by-line and converted to uppercase.
 * Invalid commands display help text.
 * 
 * @note Uses global serialCmdBuffer to accumulate characters
 * @note Buffer is limited to 20 characters to prevent overflow
 * @post Updates global CHARGING and CHARGE_ON_PLUGIN flags
 * @post Provides user feedback via Serial print statements
 */
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
      else if (cmd == "RST") {
        Serial.print(F("Resettings"));
        softwareReset();
      }
      else if (cmd == "reset") {
        Serial.print(F("Resettings"));
        softwareReset();
      }
      else if (cmd == "RESET") {
        Serial.print(F("Resettings"));
        softwareReset();
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

/**
 * @brief Monitors thermistor temperatures for overheat protection
 * 
 * Reads analog values from four thermistor inputs and checks against
 * the THERMISTER_THRESHOLD. If any sensor exceeds the threshold,
 * the system immediately stops charging and disables VSYS for safety.
 * 
 * @note Thermistors connected to pins PA2, PA3, PA4, PA5
 * @note Threshold comparison uses raw ADC values (0-4095 range)
 * @warning System enters error state if overheating is detected
 * @post Sets global ERRORFLG and OVERHEAT flags if threshold exceeded
 * @post Disables charging and VSYS output for safety
 */
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

/**
 * @brief Performs final configuration check and saves settings to EEPROM
 * 
 * This function detaches the timer interrupt, saves all configuration parameters
 * to EEPROM, marks the system as configured, and restarts the system check timer.
 * It acts as the final step in the configuration process.
 * 
 * @note This function is called when all configuration parameters have been received
 * @note The timer is temporarily detached to prevent conflicts during EEPROM operations
 * 
 * @see EEPROM_Check()
 * @see ConfigAndStartBQ()
 */
void ConfigCheck(){
  ITimer2.detachInterrupt();
  //ITimer2.attachInterruptInterval(5000000, SystemCheck);
  //Serial.print("Config status:");Serial.print(configstatus);Serial.println("/8");
  /*
  if(configstatus >= 7){
    CONFIGURED = true;
    Serial.println("Namaste bitches");
    ITimer2.attachInterruptInterval(5000000, SystemCheck);
  }
  else{
    CONFIGURED = false;
    Serial.println("Fuck you");
    ITimer2.attachInterruptInterval(5000000, SystemCheck);
  }*/
  
  EEPROM_CONFIG_FLAG = 1;
  EEPROM.put(EEPROM_ADDR_1, ChargeVoltageValue);
  EEPROM.put(EEPROM_ADDR_2, VsysMinValue);
  EEPROM.put(EEPROM_ADDR_3, inputVoltageValue);
  EEPROM.put(EEPROM_ADDR_4, chargeCurrentValue);
  EEPROM.put(EEPROM_ADDR_5, inputCurrentLimitValue);
  EEPROM.put(EEPROM_ADDR_6, CellMaxCutOffV);
  EEPROM.put(EEPROM_ADDR_7, CellMinCutOffV);
  EEPROM.put(EEPROM_ADDR_8, Pack_stock_capacity);
  EEPROM.put(EEPROM_ADDR_9, EEPROM_CONFIG_FLAG);
  
  CONFIGURED = true;
  ConfigAndStartBQ();
  Serial.print("");
  ITimer2.attachInterruptInterval(5000000, SystemCheck);
}

/**
 * @brief I2C receive event handler for command processing
 * 
 * This function handles incoming I2C data from the master device. It processes
 * two types of commands:
 * - Single byte commands (0x01-0x08): Request for sensor readings
 * - Five byte commands (0x10-0x19): Configuration parameter updates
 * 
 * @param howMany Number of bytes received via I2C
 * 
 * @details Single byte commands:
 * - 0x01: Cell 1 voltage (PackVoltage - INA_0x44_VOLTAGE)
 * - 0x02: Cell 2 voltage (INA_0x44_VOLTAGE - INA_0x41_VOLTAGE)
 * - 0x03: Cell 3 voltage (INA_0x41_VOLTAGE - INA_0x40_VOLTAGE)
 * - 0x04: Cell 4 voltage (INA_0x40_VOLTAGE)
 * - 0x05: Total pack voltage
 * - 0x06: Pack current
 * - 0x08: Real charge in mAh
 * 
 * @details Five byte commands (command + 4-byte float):
 * - 0x10: Charge voltage value
 * - 0x11: System minimum voltage
 * - 0x13: Input voltage value
 * - 0x14: Charge current value
 * - 0x15: Input current limit
 * - 0x16: Cell maximum cutoff voltage
 * - 0x17: Cell minimum cutoff voltage
 * - 0x18: Pack stock capacity (triggers ConfigCheck())
 * - 0x19: Configuration status from master
 * 
 * @note Timer interrupt is temporarily detached during parameter updates to prevent conflicts
 * @warning Unknown commands will set responseData to -1.0
 */
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
        //EEPROM.put(EEPROM_ADDR_1, ChargeVoltageValue);
        //delay(10);
        //configstatus++;
        break;
      case 0x11:
        VsysMinValue = receivedFloat;
        Serial.print("VsysMinValue Recieved From Peripheral:  ");Serial.println(VsysMinValue);
        //EEPROM.put(EEPROM_ADDR_2, VsysMinValue);
        //delay(10);
        //configstatus++;
        break;
      case 0x13:
        inputVoltageValue = receivedFloat;
        Serial.print("inputVoltageValue Recieved From Peripheral:  ");Serial.println(inputVoltageValue);
        //EEPROM.put(EEPROM_ADDR_3, inputVoltageValue);
        //delay(50);
        //configstatus++;
        break;
      case 0x14:
        chargeCurrentValue = receivedFloat;
        Serial.print("chargeCurrentValue Recieved From Peripheral:  ");Serial.println(chargeCurrentValue);
        //EEPROM.put(EEPROM_ADDR_4, chargeCurrentValue);
        //delay(50);
        //configstatus++;
        break;
      case 0x15:
        inputCurrentLimitValue = receivedFloat;
        Serial.print("inputCurrentLimitValue Recieved From Peripheral:  ");Serial.println(inputCurrentLimitValue);
        //EEPROM.put(EEPROM_ADDR_5, inputCurrentLimitValue);
        //delay(50);
        //configstatus++;
        break;
      case 0x16:
        CellMaxCutOffV = receivedFloat;
        CellFullChargeV = CellMaxCutOffV - 0.1;
        Serial.print("CellMaxCutOffV Recieved From Peripheral:  ");Serial.println(CellMaxCutOffV);
        //EEPROM.put(EEPROM_ADDR_6, CellMaxCutOffV);
        //delay(50);
        //configstatus++;
        break;
      case 0x17:
        CellMinCutOffV = receivedFloat;
        delay(10);
        Serial.print("CellMinCutOffV Recieved From Peripheral:  ");Serial.println(CellMinCutOffV);
        //EEPROM.put(EEPROM_ADDR_7, CellMinCutOffV);
        //delay(10);
        //configstatus++;
        break;
      case 0x18:
        Pack_stock_capacity = receivedFloat;
        Serial.print("Pack_stock_capacity Recieved From Peripheral:  ");Serial.println(Pack_stock_capacity);
        //EEPROM.put(EEPROM_ADDR_8, Pack_stock_capacity);
        //delay(50);
        //Serial.println("\n\n\n\n\n\nLine 967\n\n\n\n\n\n\n");
        //configstatus++;
        ConfigCheck();
        break;
        case 0x19:
        //Serial.println("\n\n\n\n\n\nLine 972\n\n\n\n\n\n\n");
        ConfigStatusFromMaster = receivedFloat;
        //Serial.println("\n\n\n\n\n\nLine 974\n\n\n\n\n\n\n");
        Serial.print("Configuration status from Master: ");Serial.println(ConfigStatusFromMaster);
        if(ConfigStatusFromMaster == 1)
        {
          Serial.println("Master reported Configuration complete");
          //configstatus = 8;
          
          ConfigCheck();
        }
        break;
      default:
        Serial.println("Unknown float write command");
        break;
    }
  }
}

/**
 * @brief I2C request event handler for sending response data
 * 
 * This function is called when the master device requests data from this slave.
 * It sends the current responseData value (set by onReceive) and resets it to 0.0.
 * 
 * @note responseData is automatically reset to 0.0 after each transmission
 * @see onReceive()
 */
void onRequest() {
  myI2C2.write((uint8_t *)&responseData, sizeof(responseData));
  responseData = 0.0;
}

/**
 * @brief Checks and loads configuration from EEPROM if available
 * 
 * This function verifies if the EEPROM contains valid configuration data.
 * If valid data is found, it loads all configuration parameters and marks
 * the system as configured. If no valid data is found, the system remains
 * in unconfigured state.
 * 
 * @note This function is called during system startup
 * @note Automatically calls ConfigAndStartBQ() and SystemCheck() if configured
 * 
 * @see isEEPROMInitialized()
 * @see ConfigAndStartBQ()
 * @see SystemCheck()
 */
void EEPROM_Check(){

  if (isEEPROMInitialized(EEPROM_ADDR_1, EEPROM_TOTAL_LENGTH)) {
    //delay(50);
    //Serial.print("\n\n\n\n\-------- Made it into EEPROM_Check Function ---------\n\n\n\n\n");
    // Read stored values from flash
    
    EEPROM.get(EEPROM_ADDR_1, ChargeVoltageValue);
    EEPROM.get(EEPROM_ADDR_2, VsysMinValue);
    EEPROM.get(EEPROM_ADDR_3, inputVoltageValue);
    EEPROM.get(EEPROM_ADDR_4, chargeCurrentValue);
    EEPROM.get(EEPROM_ADDR_5, inputCurrentLimitValue);
    EEPROM.get(EEPROM_ADDR_6, CellMaxCutOffV);
    EEPROM.get(EEPROM_ADDR_7, CellMinCutOffV);
    EEPROM.get(EEPROM_ADDR_8, Pack_stock_capacity);
    
    CONFIGURED = true;
    ConfigAndStartBQ();
    SystemCheck();
    /*Serial.print("Value read storedValue1 from flash: ");
    Serial.println(storedValue1);
    Serial.print("Value read storedValue2 from flash: ");
    Serial.println(storedValue2);*/
  } else {
    
    Serial.println("EEPROM is uninitialized or blank.");
    CONFIGURED = false;
    SystemCheck();
  }

}

/**
 * @brief Checks if EEPROM contains valid (non-blank) data
 * 
 * This function scans a specified range of EEPROM addresses to determine
 * if the memory contains valid configuration data. EEPROM is considered
 * uninitialized if all bytes are 0xFF (blank/erased state).
 * 
 * @param startAddr Starting address to check in EEPROM
 * @param length Number of bytes to check from startAddr
 * @return true if EEPROM contains data (at least one byte != 0xFF)
 * @return false if EEPROM is blank (all bytes == 0xFF)
 * 
 * @note 0xFF is the default erased state of EEPROM
 */
bool isEEPROMInitialized(int startAddr, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (EEPROM.read(startAddr + i) != 0xFF) {
      //Serial.print("EEPROM has data");
      return true;  // Found non-blank byte
      
    }
  }
  //Serial.print("EEPROM has NO data");
  return false;  // All bytes are 0xFF → uninitialized
  
}

/**
 * @brief Clears all EEPROM data by setting all bytes to 0xFF
 * 
 * This function erases all configuration data stored in EEPROM by writing
 * 0xFF to every byte in the configured EEPROM range. This effectively
 * resets the system to an unconfigured state.
 * 
 * @note System reboot is required after calling this function
 * @warning This operation is irreversible - all stored configuration will be lost
 * @see EEPROM_TOTAL_LENGTH
 */
void clearEEPROM() {
  for (int i = 0; i < EEPROM_TOTAL_LENGTH; i++) {
    EEPROM.write(i, 0xFF);
  }
  Serial.println("EEPROM wiped to 0xFF. Reboot to confirm.");
}

/**
 * @brief System initialization and setup function
 * 
 * This function performs all necessary system initialization including:
 * - Serial communication setup
 * - I2C bus initialization
 * - EEPROM configuration check and loading
 * - I2C slave setup with event handlers
 * - Hardware component initialization (LTC2943, BQ, INA sensors)
 * - GPIO pin configuration
 * - Timer interrupt setup for periodic system checks
 * 
 * @note 5-second delay at start allows for stable power-up
 * @note System check runs every 5 seconds (5,000,000 microseconds)
 * @note EEPROM is cleared at startup (clearEEPROM() call)
 * 
 * @see EEPROM_Check()
 * @see onReceive()
 * @see onRequest()
 * @see SystemCheck()
 */
void setup() {
  delay(5000);
  Serial.begin(9600);
  while (!Serial);
  //delay(50);
  Wire.begin();
  //delay(50);


  clearEEPROM();
  EEPROM.get(EEPROM_ADDR_9, EEPROM_CONFIG_FLAG);
  if(EEPROM_CONFIG_FLAG == 1 )
  {
    /*EEPROM.get(EEPROM_ADDR_1, ChargeVoltageValue);
    EEPROM.get(EEPROM_ADDR_2, VsysMinValue);
    EEPROM.get(EEPROM_ADDR_3, inputVoltageValue);
    EEPROM.get(EEPROM_ADDR_4, chargeCurrentValue);
    EEPROM.get(EEPROM_ADDR_5, inputCurrentLimitValue);
    EEPROM.get(EEPROM_ADDR_6, CellMaxCutOffV);
    EEPROM.get(EEPROM_ADDR_7, CellMinCutOffV);
    EEPROM.get(EEPROM_ADDR_8, Pack_stock_capacity);
    CONFIGURED = true;*/
    Serial.println("Grabbing Config from EEPROM, Mashallah");
    EEPROM_Check();
  }
  
  
  myI2C2.begin(SLAVE_ADDRESS);
  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);
  
  setupLTC2943();
  CheckBQConnectionWithComms();
  CheckIfINAConnected();

  EnableGPIOPins();


  // Start the timer interrupt to trigger every 500,000 microseconds (5000 ms)
  if (ITimer2.attachInterruptInterval(5000000, SystemCheck)) {
    Serial.println("Timer2 started, checking sensors every 500 ms");
  } else {
    Serial.println("Failed to start Timer2");
  }
}

/**
 * @brief Main system monitoring and control function (Timer ISR)
 * 
 * This function serves as the main system loop, called every 5 seconds by
 * the timer interrupt. It performs comprehensive system monitoring and control:
 * 
 * When CONFIGURED == true:
 * - Handles serial charging input commands
 * - Reports charging status
 * - Executes charge and balance control algorithms
 * - Monitors battery temperature
 * - Restarts the timer for next cycle
 * 
 * When CONFIGURED == false:
 * - Displays "AWAITING CONFIGURATION" message
 * - Stops timer to prevent unnecessary cycles
 * 
 * @note Timer is temporarily detached at start to prevent re-entry
 * @note Function only executes full monitoring when system is configured
 * @note Serial output provides detailed system status information
 * 
 * @see handleSerialCharging()
 * @see ChargeAndBalanceControl()
 * @see TempCheck()
 */
void SystemCheck()
{
  Serial.println("----------------------------------------------------");
  ITimer2.detachInterrupt();
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
  ITimer2.attachInterruptInterval(5000000, SystemCheck);
  }
  else{
    Serial.println("            AWAITING CONFIGURATION");
    ITimer2.detachInterrupt();
    //Serial.print("Config Flag:");Serial.println(CONFIGURED);
    //Serial.print("Config status:");Serial.print(configstatus);Serial.println("/8");
  }
  

  Serial.println("----------------------------------------------------");
  Serial.println("");
  
}

/**
 * @brief Main program loop (intentionally empty)
 * 
 * The main loop is intentionally left empty as all system functionality
 * is handled by the timer interrupt (SystemCheck()) and I2C event handlers.
 * A NOP (no operation) instruction is used to prevent compiler optimization.
 * 
 * @note All system operations are interrupt-driven
 * @see SystemCheck()
 * @see onReceive()
 * @see onRequest()
 */
void loop() {
  __asm__("nop");
}
//EOF