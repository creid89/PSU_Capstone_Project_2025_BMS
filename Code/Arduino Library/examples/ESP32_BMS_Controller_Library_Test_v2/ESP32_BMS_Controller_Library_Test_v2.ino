/**
 * @file main.cpp
 * @brief Battery Management System monitoring and configuration program
 * @author Your Name
 * @date 2025
 * 
 * This program interfaces with a BMS controller via I2C to configure charging
 * parameters and continuously monitor battery cell voltages, charging current,
 * and state of charge.
 */

#include <BMSController.h>

// Using Huzzah Adafruit ESP32
//#define SDA_PIN 23  ///< SDA pin for I2C communication (default ESP32 pins)
//#define SCL_PIN 22  ///< SCL pin for I2C communication (default ESP32 pins)

#define BMS_ADDR 0x58  ///< I2C address of the BMS controller

BMSController BMS(BMS_ADDR);  ///< BMS controller instance

/**
 * @brief Arduino setup function - runs once at startup
 * 
 * Initializes serial communication, establishes connection with BMS,
 * and configures all BMS parameters including charging voltage, current
 * limits, cell voltage cutoffs, and battery capacity.
 */
void setup() {
  Serial.begin(115200);
  
  // Wait for BMS to respond
  while (!BMS.begin()) {
    Serial.println();
    Serial.println("Waiting for BMS to respond...");
    delay(2000);
  }
  
  Serial.println("——————————————————————");
  Serial.println("          BMS connected!");
  Serial.println("——————————————————————");
  delay(500);
  
  // Configure BMS parameters
  BMS.setChargeVoltage(16.8);       ///< Set charge voltage to 16.8V (command 0x10)
  delay(100);
  BMS.setVsysMin(8.0);              ///< Set minimum system voltage to 8.0V (command 0x11)
  delay(100);
  BMS.setInputVoltageLimit(12.5);   ///< Set input voltage limit to 12.5V (command 0x13)
  delay(100);
  BMS.setChargeCurrent(1.5);        ///< Set charge current to 1.5A (command 0x14)
  delay(100);
  BMS.setInputCurrentLimit(3000.0); ///< Set input current limit to 3000mA (command 0x15)
  delay(100);
  BMS.setCellMaxCutOffV(4.0);       ///< Set maximum cell voltage cutoff to 4.0V (command 0x16)
  delay(100);
  BMS.setCellMinCutOffV(2.0);       ///< Set minimum cell voltage cutoff to 2.0V (command 0x17)
  delay(100);
  BMS.setPack_stock_capacity(5200.0); ///< Set battery pack capacity to 5200mAh (command 0x18)
  
  // Uncomment to signal configuration completion
  //BMS.FinishedConfiguration(true);  ///< Signal configuration complete (command 0x19)
  
  Serial.println("——————————————————————");
  Serial.println("          BMS Configured!");
  Serial.println("——————————————————————");
}

/**
 * @brief Arduino main loop function - runs continuously
 * 
 * Continuously monitors and displays BMS status including:
 * - Charging current
 * - Individual cell voltages (4 cells)
 * - Total pack voltage
 * - State of charge (absolute and percentage)
 * 
 * All values are displayed with 3 decimal places precision.
 * Each reading is separated by a 1.5 second delay.
 */
void loop() {
  delay(1500);
  
  // Read and display charging current
  float chargeCurrent = BMS.getChargeCurrent();
  Serial.print("Charge Current: ");
  Serial.println(chargeCurrent, 3);
  
  delay(1500);
  
  // Read and display individual cell voltages
  float cellVoltage1 = BMS.getCell1Voltage();
  Serial.print("Cell 1 Voltage: ");
  Serial.println(cellVoltage1, 3);
  
  delay(1500);
  
  float cellVoltage2 = BMS.getCell2Voltage();
  Serial.print("Cell 2 Voltage: ");
  Serial.println(cellVoltage2, 3);
  
  delay(1500);
  
  float cellVoltage3 = BMS.getCell3Voltage();
  Serial.print("Cell 3 Voltage: ");
  Serial.println(cellVoltage3, 3);
  
  delay(1500);
  
  float cellVoltage4 = BMS.getCell4Voltage();
  Serial.print("Cell 4 Voltage: ");
  Serial.println(cellVoltage4, 3);
  
  delay(1500);
  
  // Read and display total pack voltage
  float totalPackVoltage = BMS.getTotalPackVoltage();
  Serial.print("Total Pack Voltage: ");
  Serial.println(totalPackVoltage, 3);
  
  delay(1500);
  
  // Read and display state of charge (absolute value)
  float SoC = BMS.getSoC();
  Serial.print("SoC: ");
  Serial.println(SoC, 3);
  
  delay(1500);
  
  // Read and display state of charge as percentage
  float PercentSoC = BMS.getPercentSoC();
  Serial.print("Percent SoC: ");
  Serial.print(PercentSoC, 3);
  Serial.println("%");
}
