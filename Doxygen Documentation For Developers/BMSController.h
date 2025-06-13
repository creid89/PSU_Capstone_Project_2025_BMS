/**
 * @file BMSController.h
 * @brief Battery Management System Controller Library
 * @author Your Name
 * @date 2025
 */

#ifndef BMSController_h
#define BMSController_h

#include <Arduino.h>
#include <Wire.h>

/**
 * @class BMSController
 * @brief A class to control and monitor a Battery Management System via I2C communication
 * 
 * This class provides methods to configure charging parameters, voltage limits,
 * current limits, and monitor individual cell voltages and state of charge.
 */
class BMSController {
  public:
    /**
     * @brief Constructor for BMSController
     * @param i2cAddress The I2C address of the BMS device
     */
    BMSController(uint8_t i2cAddress);
    
    /**
     * @brief Initialize the BMS controller and establish I2C communication
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Set the target charging voltage
     * @param volts The charging voltage in volts
     * @return true if command sent successfully, false otherwise
     */
    bool setChargeVoltage(float volts);
    
    /**
     * @brief Set the minimum system voltage
     * @param volts The minimum system voltage in volts
     * @return true if command sent successfully, false otherwise
     */
    bool setVsysMin(float volts);
    
    /**
     * @brief Set the input voltage limit
     * @param volts The input voltage limit in volts
     * @return true if command sent successfully, false otherwise
     */
    bool setInputVoltageLimit(float volts);
    
    /**
     * @brief Set the charging current limit
     * @param amps The charging current limit in amperes
     * @return true if command sent successfully, false otherwise
     */
    bool setChargeCurrent(float amps);
    
    /**
     * @brief Set the input current limit
     * @param milliamps The input current limit in milliamperes
     * @return true if command sent successfully, false otherwise
     */
    bool setInputCurrentLimit(float milliamps);
    
    /**
     * @brief Set the maximum cell voltage cutoff threshold
     * @param volts The maximum cell voltage cutoff in volts
     * @return true if command sent successfully, false otherwise
     */
    bool setCellMaxCutOffV(float volts);
    
    /**
     * @brief Set the minimum cell voltage cutoff threshold
     * @param volts The minimum cell voltage cutoff in volts
     * @return true if command sent successfully, false otherwise
     */
    bool setCellMinCutOffV(float volts);
    
    /**
     * @brief Set the battery pack stock capacity
     * @param mAh The battery pack capacity in milliampere-hours
     * @return true if command sent successfully, false otherwise
     */
    bool setPack_stock_capacity(float mAh);
    
    /**
     * @brief Signal that configuration is complete
     * @param completed true if configuration is finished, false otherwise
     * @return true if command sent successfully, false otherwise
     */
    bool FinishedConfiguration(bool completed);
    
    /**
     * @brief Get the current charging current
     * @return The charging current in amperes
     */
    float getChargeCurrent();
    
    /**
     * @brief Get the voltage of cell 1
     * @return The voltage of cell 1 in volts
     */
    float getCell1Voltage();
    
    /**
     * @brief Get the voltage of cell 2
     * @return The voltage of cell 2 in volts
     */
    float getCell2Voltage();
    
    /**
     * @brief Get the voltage of cell 3
     * @return The voltage of cell 3 in volts
     */
    float getCell3Voltage();
    
    /**
     * @brief Get the voltage of cell 4
     * @return The voltage of cell 4 in volts
     */
    float getCell4Voltage();
    
    /**
     * @brief Get the total battery pack voltage
     * @return The total pack voltage in volts
     */
    float getTotalPackVoltage();
    
    /**
     * @brief Get the State of Charge in absolute units
     * @return The State of Charge value
     */
    float getSoC();
    
    /**
     * @brief Get the State of Charge as a percentage
     * @return The State of Charge as a percentage (0-100%)
     */
    float getPercentSoC();
    
  private:
    uint8_t _address; ///< I2C address of the BMS device
    
    /**
     * @brief Send a float value to the BMS via I2C
     * @param command The command byte to send
     * @param value The float value to transmit
     */
    void sendFloat(uint8_t command, float value);
    
    /**
     * @brief Retrieve a float value from the BMS via I2C
     * @param command The command byte to request data
     * @return The retrieved float value
     */
    float getValue(uint8_t command);
};

#endif