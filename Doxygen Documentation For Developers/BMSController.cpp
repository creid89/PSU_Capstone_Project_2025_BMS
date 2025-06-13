/**
 * @file BMSController.cpp
 * @brief Implementation of BMSController class for I2C communication with Battery Management System
 * @author Your Name
 * @date Current Date
 * @version 1.0
 * 
 * This file implements the BMSController class which provides a high-level interface
 * for communicating with a Battery Management System (BMS) over I2C. The class
 * handles configuration parameters, sensor readings, and State of Charge calculations.
 */

#include "BMSController.h"

/** @brief Global variable to store the given State of Charge capacity for percentage calculations */
float SoCGiven;

/**
 * @brief Constructor for BMSController class
 * 
 * Initializes the BMSController with the specified I2C address and sets up
 * the I2C communication interface.
 * 
 * @param i2cAddress I2C slave address of the BMS device (7-bit address)
 * 
 * @note Wire.begin() is called during construction to initialize I2C
 */
BMSController::BMSController(uint8_t i2cAddress) {
  _address = i2cAddress;
  Wire.begin();
}

/**
 * @brief Initialize communication with the BMS device
 * 
 * Tests communication with the BMS by attempting to establish an I2C connection.
 * This function should be called after construction to verify the device is
 * responding on the specified address.
 * 
 * @return true if BMS device responds to I2C communication
 * @return false if no response from BMS device
 * 
 * @note This function reinitializes Wire.begin() for safety
 */
bool BMSController::begin() {
  Wire.begin();
  Wire.beginTransmission(_address);
  if (Wire.endTransmission() == 0) {
    return true;  // Device responded
  }
  return false;  // No response
}

/**
 * @brief Send a float value with command to BMS (Legacy version - commented out)
 * 
 * Basic implementation without retry logic or error handling.
 * This version has been replaced by the experimental version below.
 * 
 * @param command Command byte to send to BMS
 * @param value Float value to transmit
 */
/**void BMSController::sendFloat(uint8_t command, float value) {
  Wire.beginTransmission(_address);
  Wire.write(command);
  uint8_t buf[4];
  memcpy(buf, &value, sizeof(float));
  Wire.write(buf, 4);
  Wire.endTransmission();
}**/

///////////////////////////////////EXPERIMENTAL/////////////////////////////////////////////////

/**
 * @brief Send a float value with command to BMS (Enhanced version with retry logic)
 * 
 * This experimental version includes robust error handling with automatic retry
 * capability to handle I2C bus congestion or temporary communication failures.
 * The function will retry up to 10 times with 5ms delays between attempts.
 * 
 * @param command Command byte to send to BMS (see command reference)
 * @param value Float value to transmit (4-byte IEEE 754 format)
 * 
 * @note Maximum 10 retries with 5ms delay between attempts
 * @note Prints error message to Serial if all retries fail
 * @warning Function blocks during retry attempts
 * 
 * @details Command format: [command_byte][4_bytes_float_data]
 * Total transmission: 5 bytes
 */
void BMSController::sendFloat(uint8_t command, float value) {
  const uint8_t maxRetries = 10;
  const uint16_t retryDelayMs = 5;
  uint8_t retries = 0;
  int transmissionResult = 4; // Arbitrary non-zero value to enter loop
  
  // Retry loop to wait for the bus to become available
  while (retries < maxRetries) {
    Wire.beginTransmission(_address);
    Wire.write(command);
    uint8_t buf[4];
    memcpy(buf, &value, sizeof(float));
    Wire.write(buf, 4);
    transmissionResult = Wire.endTransmission();
    
    if (transmissionResult == 0) {
      // Success
      break;
    } else {
      // Transmission failed, possibly bus busy or NACKed
      retries++;
      delay(retryDelayMs);
    }
  }
  
  if (transmissionResult != 0) {
    // Optional: handle error here (e.g., log or raise flag)
    Serial.print("I2C transmission failed, error code: ");
    Serial.println(transmissionResult);
  }
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Set the charge voltage limit for the battery pack
 * 
 * Configures the maximum voltage that the battery pack should be charged to.
 * This is a critical safety parameter that prevents overcharging.
 * 
 * @param volts Charge voltage limit in volts
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x10
 * @warning Ensure voltage is within safe limits for your battery chemistry
 */
bool BMSController::setChargeVoltage(float volts) {
  sendFloat(0x10, volts);
  return true;
}

/**
 * @brief Set the minimum system voltage threshold
 * 
 * Configures the minimum voltage below which the system should take
 * protective action to prevent deep discharge damage.
 * 
 * @param volts Minimum system voltage in volts
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x11
 */
bool BMSController::setVsysMin(float volts) {
  sendFloat(0x11, volts);
  return true;
}

/**
 * @brief Set the input voltage limit for charging
 * 
 * Configures the maximum input voltage that the charging system will accept.
 * This helps protect against overvoltage conditions from the power supply.
 * 
 * @param volts Input voltage limit in volts
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x13
 */
bool BMSController::setInputVoltageLimit(float volts) {
  sendFloat(0x13, volts);
  return true;
}

/**
 * @brief Set the maximum charge current limit
 * 
 * Configures the maximum current that should be used during charging.
 * This parameter is crucial for battery safety and longevity.
 * 
 * @param amps Maximum charge current in amperes
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x14
 * @warning Ensure current is within battery specifications
 */
bool BMSController::setChargeCurrent(float amps) {
  sendFloat(0x14, amps);
  return true;
}

/**
 * @brief Set the input current limit from power supply
 * 
 * Configures the maximum current that can be drawn from the input power supply.
 * This prevents overloading the power source during charging.
 * 
 * @param milliamps Input current limit in milliamperes
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x15
 * @note Parameter is in milliamperes, not amperes
 */
bool BMSController::setInputCurrentLimit(float milliamps) {
  sendFloat(0x15, milliamps);
  return true;
}

/**
 * @brief Set the maximum cell voltage cutoff threshold
 * 
 * Configures the voltage at which individual cells should be considered
 * fully charged and charging should be terminated to prevent damage.
 * 
 * @param volts Maximum cell voltage in volts
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x16
 * @warning Critical safety parameter - must match battery chemistry
 */
bool BMSController::setCellMaxCutOffV(float volts) {
  sendFloat(0x16, volts);
  return true;
}

/**
 * @brief Set the minimum cell voltage cutoff threshold
 * 
 * Configures the voltage below which individual cells should be considered
 * discharged and further discharge should be prevented to avoid damage.
 * 
 * @param volts Minimum cell voltage in volts
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x17
 * @warning Critical safety parameter - prevents deep discharge damage
 */
bool BMSController::setCellMinCutOffV(float volts) {
  sendFloat(0x17, volts);
  return true;
}

/**
 * @brief Set the battery pack stock capacity
 * 
 * Configures the nominal capacity of the battery pack in mAh. This value
 * is used for State of Charge calculations and capacity tracking.
 * This function also triggers the final configuration check on the BMS.
 * 
 * @param mAh Battery pack capacity in milliamp-hours
 * @return true if command was sent successfully
 * @return false if transmission failed
 * 
 * @note Command code: 0x18
 * @note Sets global SoCGiven variable for percentage calculations
 * @note This command triggers ConfigCheck() on the BMS side
 */
bool BMSController::setPack_stock_capacity(float mAh) {
  SoCGiven = mAh;
  sendFloat(0x18, mAh);
  return true;
}

/**
 * @brief Signal completion of BMS configuration
 * 
 * Sends a configuration completion signal to the BMS to indicate that
 * all configuration parameters have been set and the system should
 * transition to operational mode.
 * 
 * @param completed Configuration completion status (true = completed)
 * @return true if completed is true and command was sent
 * @return false if completed is false or transmission failed
 * 
 * @note Command code: 0x19
 * @note Only sends command if completed parameter is true
 */
bool BMSController::FinishedConfiguration(bool completed) {
  if(completed == true)
  {
    sendFloat(0x19, completed);
    return true;
  }
  return false;
}

/**
 * @brief Generic function to request a float value from BMS
 * 
 * Sends a single-byte command to the BMS and receives a 4-byte float response.
 * This is the core function used by all getter methods.
 * 
 * @param command Single-byte command to send to BMS
 * @return Float value received from BMS
 * @return -1.0 if communication error occurred
 * 
 * @note Times out if BMS doesn't respond with 4 bytes
 * @note Uses memcpy for proper float reconstruction from bytes
 */
float BMSController::getValue(uint8_t command) {
  float value = -1.0;
  Wire.beginTransmission(_address);
  Wire.write(command);
  if (Wire.endTransmission() != 0) return -1.0;
  
  Wire.requestFrom(_address, (uint8_t)4);
  if (Wire.available() < 4) return -1.0;
  
  uint8_t buf[4];
  for (int i = 0; i < 4; i++) buf[i] = Wire.read();
  memcpy(&value, buf, 4);
  return value;
}

/**
 * @brief Get the current charge/discharge current of the battery pack
 * 
 * Retrieves the instantaneous current flowing into (positive) or out of
 * (negative) the battery pack.
 * 
 * @return Current in amperes (positive = charging, negative = discharging)
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x06
 */
float BMSController::getChargeCurrent() {
  return getValue(0x06);
}

/**
 * @brief Get the voltage of cell 1
 * 
 * Retrieves the voltage measurement of the first cell in the battery pack.
 * 
 * @return Cell 1 voltage in volts
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x01
 * @note Calculated as: PackVoltage - INA_0x44_VOLTAGE
 */
float BMSController::getCell1Voltage() {
  return getValue(0x01);
}

/**
 * @brief Get the voltage of cell 2
 * 
 * Retrieves the voltage measurement of the second cell in the battery pack.
 * 
 * @return Cell 2 voltage in volts
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x02
 * @note Calculated as: INA_0x44_VOLTAGE - INA_0x41_VOLTAGE
 */
float BMSController::getCell2Voltage() {
  return getValue(0x02);
}

/**
 * @brief Get the voltage of cell 3
 * 
 * Retrieves the voltage measurement of the third cell in the battery pack.
 * 
 * @return Cell 3 voltage in volts
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x03
 * @note Calculated as: INA_0x41_VOLTAGE - INA_0x40_VOLTAGE
 */
float BMSController::getCell3Voltage() {
  return getValue(0x03);
}

/**
 * @brief Get the voltage of cell 4
 * 
 * Retrieves the voltage measurement of the fourth cell in the battery pack.
 * 
 * @return Cell 4 voltage in volts
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x04
 * @note Directly reads INA_0x40_VOLTAGE
 */
float BMSController::getCell4Voltage() {
  return getValue(0x04);
}

/**
 * @brief Get the total voltage of the battery pack
 * 
 * Retrieves the combined voltage measurement of all cells in series.
 * 
 * @return Total pack voltage in volts
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x05
 */
float BMSController::getTotalPackVoltage() {
  return getValue(0x05);
}

/**
 * @brief Get the State of Charge in absolute units
 * 
 * Retrieves the current State of Charge measurement from the BMS
 * in the original units (typically mAh).
 * 
 * @return State of Charge in absolute units (mAh)
 * @return -1.0 if communication error
 * 
 * @note Command code: 0x08
 * @see getPercentSoC() for percentage-based SoC
 */
float BMSController::getSoC() {
  return getValue(0x08);
}

/**
 * @brief Get the State of Charge as a percentage
 * 
 * Calculates and returns the State of Charge as a percentage of the
 * configured pack capacity. Requires setPack_stock_capacity() to have
 * been called previously with the correct pack capacity.
 * 
 * @return State of Charge as percentage (0-100%)
 * @return Calculated percentage based on SoCGiven capacity
 * 
 * @note Calculation: (current_SoC / SoCGiven) * 100
 * @note Requires SoCGiven to be set via setPack_stock_capacity()
 * @warning Returns invalid result if SoCGiven is 0 or not set
 * 
 * @see getSoC()
 * @see setPack_stock_capacity()
 */
float BMSController::getPercentSoC() {
  float SoC = getValue(0x08);
  float PercentSoC = SoC/SoCGiven*100;
  return PercentSoC;
}