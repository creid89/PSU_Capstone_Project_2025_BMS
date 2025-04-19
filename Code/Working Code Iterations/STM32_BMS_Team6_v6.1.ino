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