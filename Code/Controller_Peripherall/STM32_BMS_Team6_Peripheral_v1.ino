#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>
#include "stm32f1xx.h"

// Serial2 and I2C2 pin definitions
HardwareSerial Serial2(PA3, PA2);  // RX, TX
TwoWire myI2C2(PB11, PB10);
#define SLAVE_ADDRESS 0x58

// Sensor instances
Adafruit_INA260 ina260_0x40;
Adafruit_INA260 ina260_0x41;
Adafruit_INA260 ina260_0x44;
Adafruit_INA260 ina260_0x45;

// Timer
STM32Timer ITimer2(TIM2);

// Flags
volatile bool INA260_0x40_Flag = false;
volatile bool INA260_0x41_Flag = false;
volatile bool INA260_0x44_Flag = false;
volatile bool INA260_0x45_Flag = false;

// Sensor data
float INA_0x40_VOLTAGE = 0.0;
float INA_0x41_VOLTAGE = 0.0;
float INA_0x44_VOLTAGE = 0.0;
float INA_0x45_VOLTAGE = 0.0;
float PACK_CURRENT = 0.0;

// Derived data
float CELL1_VOLTAGE = 0.0;
float CELL2_VOLTAGE = 0.0;
float CELL3_VOLTAGE = 0.0;
float CELL4_VOLTAGE = 0.0;
float TOTAL_PACK_VOLTAGE = 0.0;
float CHARGE_CURRENT = 0.0;

// BQ Charge IC Configuration parameters
float ChargeVoltageValue = 0.0;         // Charging target voltage
float VsysMinValue = 0.0;                 // Minimum system voltage
float inputVoltageValue = 0.0;            // Input voltage limit (used for VINDPM)
float chargeCurrentValue = 0.0;           // Battery charging current (in amps)
float inputCurrentLimitValue = 0.0;    // Input current limit (in milliamps) - based on supply/adapter limit

// Response to master
float responseData = 0.0;

void onTimerISR() {
  INA260_0x40_Flag = true;
  INA260_0x41_Flag = true;
  INA260_0x44_Flag = true;
  INA260_0x45_Flag = true;
}

// Handle command + optional float reception
void onReceive(int howMany) {
  if (howMany == 1) {
    uint8_t command = myI2C2.read();
    switch (command) {
      case 0x01:
        CELL1_VOLTAGE = INA_0x45_VOLTAGE - INA_0x44_VOLTAGE;
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
        responseData = INA_0x45_VOLTAGE;
        break;
      case 0x06:
        responseData = CHARGE_CURRENT;
        break;
      case 0x08:
        // Future SOC calculation
        responseData = 0.0;
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

    switch (command) {
      case 0x10:
        ChargeVoltageValue = receivedFloat;
        Serial.print("ChargeVoltageValue Recieved From Peripheral: \t");Serial.print(ChargeVoltageValue);
        break;
      case 0x11:
        VsysMinValue = receivedFloat;
        Serial.print("VsysMinValue Recieved From Peripheral: \t");Serial.print(VsysMinValue);
        break;
      case 0x12:
        VsysMinValue = receivedFloat;
        Serial.print("VsysMinValue Recieved From Peripheral: \t");Serial.print(VsysMinValue);
        break;
      case 0x13:
        inputVoltageValue = receivedFloat;
        Serial.print("inputVoltageValue Recieved From Peripheral: \t");Serial.print(inputVoltageValue);
        break;
      case 0x14:
        chargeCurrentValue = receivedFloat;
        Serial.print("chargeCurrentValue Recieved From Peripheral: \t");Serial.print(chargeCurrentValue);
        break;
      case 0x15:
        inputCurrentLimitValue = receivedFloat;
        Serial.print("inputCurrentLimitValue Recieved From Peripheral: \t");Serial.print(inputCurrentLimitValue);
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

// Sensor reads
void read_INA260_0x40_values() {
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage() / 1000.0;
  Serial.print("INA260 0x40 V = ");
  Serial.println(INA_0x40_VOLTAGE);
}

void read_INA260_0x41_values() {
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage() / 1000.0;
  Serial.print("INA260 0x41 V = ");
  Serial.println(INA_0x41_VOLTAGE);
}

void read_INA260_0x44_values() {
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage() / 1000.0;
  Serial.print("INA260 0x44 V = ");
  Serial.println(INA_0x44_VOLTAGE);
}

void read_INA260_0x45_values() {
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage() / 1000.0;
  PACK_CURRENT = ina260_0x45.readCurrent();  // in mA
  Serial.print("INA260 0x45 V = ");
  Serial.println(INA_0x45_VOLTAGE);
  Serial.print("PACK CURRENT = ");
  Serial.println(PACK_CURRENT);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  myI2C2.begin(SLAVE_ADDRESS);
  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);

  //Serial2.begin(9600);

  if (!ina260_0x40.begin(0x40)) {
    Serial.println("INA260 @ 0x40 not found"); while (1);
  }
  if (!ina260_0x41.begin(0x41)) {
    Serial.println("INA260 @ 0x41 not found"); while (1);
  }
  if (!ina260_0x44.begin(0x44)) {
    Serial.println("INA260 @ 0x44 not found"); while (1);
  }
  if (!ina260_0x45.begin(0x45)) {
    Serial.println("INA260 @ 0x45 not found"); while (1);
  }

  if (ITimer2.attachInterruptInterval(500000, onTimerISR)) {
    Serial.println("Timer started: 500ms");
  } else {
    Serial.println("Timer failed");
  }

  Serial.println("STM32 I2C Slave Ready");
}

void loop() {
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
}
