#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>
#include "stm32f1xx.h"  // Include STM32F1 device header for register access

/*
Working on casting the cell voltage value from a floating point to a hex value to send across i2c
The data being recieved on the controller side is not converting back correctly
Could be an issue with unint8_t vs uint16_t delcrations in both programs
Need to "un-cast" the value on the controller side
This version has intergrated the i2c2 bus
The serial2 bus for comms over UART is left in
the v5 of the code on github the cell balancing but this version does not
*/

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX
 
//create a new instance for the I2C2 bus to be used as a slave
TwoWire myI2C2(PB11, PB10);
#define SLAVE_ADDRESS 0x60
//create a buffer to recieve data on the i2c2 bus
//#define I2C_RX_BUFFER_SIZE 32
//uint8_t i2c_rx_buffer[I2C_RX_BUFFER_SIZE];

uint8_t responseData = 0x00;

// Create instances for each INA260 sensor
Adafruit_INA260 ina260_0x40;
Adafruit_INA260 ina260_0x41;
Adafruit_INA260 ina260_0x44;
Adafruit_INA260 ina260_0x45;

// Volatile flag set by the timer ISR to indicate that it is time to check Serial2
volatile bool serialCheckFlag = false;
volatile bool readINA_1_Flag = false;
volatile bool INA260_0x40_Flag = false;
volatile bool INA260_0x41_Flag = false;
volatile bool INA260_0x44_Flag = false;
volatile bool INA260_0x45_Flag = false;

//Global Variable
float PACK_CURRENT;
float INA_0x40_VOLTAGE = 0.0;
float INA_0x41_VOLTAGE = 0.0;
float INA_0x44_VOLTAGE = 0.0;
float INA_0x45_VOLTAGE = 0.0;
float CELL1_VOLTAGE = 0.00;
float CELL2_VOLTAGE = 0.00;
float CELL3_VOLTAGE = 0.00;
float CELL4_VOLTAGE = 0.00;

// Create a timer instance using TIM2 (adjust if needed)
STM32Timer ITimer2(TIM2);

// Timer interrupt service routine (ISR)
void onTimerISR() {
  serialCheckFlag = true;
  INA260_0x40_Flag = true;
  INA260_0x41_Flag = true;
  INA260_0x44_Flag = true;
  INA260_0x45_Flag = true;
}


void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }
  //Init i2c2 in slave mode at SLAVE_ADDRESS 0x60
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
    Serial.println("Timer2 started, checking Serial2 every 500 ms");
  } else {
    Serial.println("Failed to start Timer2");
  }

  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);
}



void loop() {
  // If the timer ISR has set the flag, process the Serial2 data
  if (serialCheckFlag) {
    serialCheckFlag = false; // Clear the flag
    readSerial2Data();         // Call the function to read and print Serial2 data
  }
  
  if (INA260_0x40_Flag) {
    INA260_0x40_Flag = false;  // Clear the flag
    //Serial.println("Timer ISR triggered, reading sensor...");
    read_INA260_0x40_values();
  }

  if (INA260_0x41_Flag) {
    INA260_0x41_Flag = false;  // Clear the flag
    //Serial.println("Timer ISR triggered, reading sensor...");
    read_INA260_0x41_values();
  }

  if (INA260_0x44_Flag) {
    INA260_0x44_Flag = false;  // Clear the flag
    //Serial.println("Timer ISR triggered, reading sensor...");
    read_INA260_0x44_values();
  }
  if (INA260_0x45_Flag) {
    INA260_0x45_Flag = false;  // Clear the flag
    //Serial.println("Timer ISR triggered, reading sensor...");
    read_INA260_0x45_values();
  }

  myI2C2.onReceive(onReceive);
  myI2C2.onRequest(onRequest);
}



void onReceive(int howMany) {
  // Limit the bytes to read to MAX_BYTES_TO_READ (2 bytes)
  uint16_t recievedBytes = myI2C2.read();

      switch(recievedBytes){
        case 0x01:
          Serial.print("Requested Voltage of Cell 1\n");
          CELL1_VOLTAGE = INA_0x45_VOLTAGE - INA_0x44_VOLTAGE;
          Serial.print("Voltage of Cell 1: ");Serial.print(CELL1_VOLTAGE);
          responseData = (uint8_t)(CELL1_VOLTAGE * 100);
          break;
        case 0x02:
          Serial.print("Requested Voltage of Cell 2\n");
          responseData = 0xAA;
          break;
        case 0x03:
          Serial.print("Requested Voltage of Cell 3\n");
          break;
        case 0x04:
          Serial.print("Requested Voltage of Cell 4\n");
          break;
        case 0x05:
          Serial.print("Requested Total Pack Voltage\n");
          //call function that measures voltage
          break;
        case 0x06:
          Serial.print("Requested Current Going Through Pack");
          //call function that measures voltage
          break;
        case 0x07:
          Serial.print("Requested Power Consumed");
          //call function that measures voltage
          break;
        case 0x08:
          Serial.print("Requested State of Charge");
          //call function that measures voltage
          break;
        default:
          Serial.print("Unknown Command Recieved\n");


  }

}
void onRequest(){

  myI2C2.write((uint8_t *)&responseData, 2);
  responseData = 0x00;

}


// Function to read and print data from Serial2
void readSerial2Data() {
    while (Serial2.available() > 0) {
      uint8_t receivedByte = Serial2.read();
      switch(receivedByte){
        case 0x01:
          Serial.print("Requested Voltage of Cell 1\n");
          CELL1_VOLTAGE = INA_0x45_VOLTAGE - INA_0x44_VOLTAGE;
          Serial.print("Voltage Cell 1 = ");
          Serial.println(CELL1_VOLTAGE);
          Serial.print(" V");
          Serial2.print(CELL1_VOLTAGE);
          break;
        case 0x02:
          Serial.print("Requested Voltage of Cell 2\n");
          CELL2_VOLTAGE = INA_0x44_VOLTAGE - INA_0x41_VOLTAGE;
          Serial.print("Voltage Cell 2 = ");
          Serial.println(CELL2_VOLTAGE);
          Serial.print(" V");
          break;
        case 0x03:
          Serial.print("Requested Voltage of Cell 3\n");
          CELL3_VOLTAGE = INA_0x41_VOLTAGE - INA_0x40_VOLTAGE;
          Serial.print("Voltage Cell 3 = ");
          Serial.println(CELL3_VOLTAGE);
          Serial.print(" V");
          break;
        case 0x04:
          Serial.print("Requested Voltage of Cell 4\n");
          CELL4_VOLTAGE = INA_0x40_VOLTAGE;
          Serial.print("Voltage Cell 4 = ");
          Serial.println(CELL4_VOLTAGE);
          Serial.print(" V");

          break;
        case 0x05:
          Serial.print("Requested Total Pack Voltage\n");
          //call function that measures voltage
          break;
        case 0x06:
          Serial.print("Requested Current Going Through Pack");
          //call function that measures voltage
          break;
        case 0x07:
          Serial.print("Requested Power Consumed");
          //call function that measures voltage
          break;
        case 0x08:
          Serial.print("Requested State of Charge");
          //call function that measures voltage
          break;
        default:
          Serial.print("Unknown Command Recieved\n");
          break;
          
      }
    }
}

void read_INA260_0x40_values() {
  //float current_mA = ina260_0x40.readCurrent();     // Current in mA
  //float voltage_V  = ina260_0x40.readBusVoltage();    // Voltage in V
  //float power_mW   = ina260_0x40.readPower();         // Power in mW
  delay(1000);
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage()/1000.00;
  Serial.print("\n Voltage of ina260_0x40 = ");
  Serial.println(INA_0x40_VOLTAGE); 


}

void read_INA260_0x41_values() {
  //float current_mA = ina260_0x41.readCurrent();     // Current in mA
  //float voltage_V  = ina260_0x41.readBusVoltage();    // Voltage in V
  //float power_mW   = ina260_0x41.readPower();         // Power in mW
  delay(1000);
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage()/1000.00;
  Serial.print("Voltage of ina260_0x41 = ");
  Serial.println(INA_0x41_VOLTAGE); 
  

}

void read_INA260_0x44_values() {
  //float current_mA = ina260_0x44.readCurrent();     // Current in mA
  //float voltage_V  = ina260_0x44.readBusVoltage();    // Voltage in V
  //float power_mW   = ina260_0x44.readPower();         // Power in mW
  delay(1000);
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage()/1000.00;
  Serial.print("Voltage of ina260_0x44 = ");
  Serial.println(INA_0x44_VOLTAGE);
}

void read_INA260_0x45_values() {
  //this function is used measure current flowing into the pack
  //This will constantly update the global float
  delay(1000);
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage()/1000.00;
  Serial.print("Voltage of ina260_0x45 = ");
  Serial.println(INA_0x45_VOLTAGE);
  delay(1000);  
  PACK_CURRENT = (ina260_0x45.readCurrent())/1000.00; //read current in A
  Serial.print("Current Through the pack = ");
  Serial.println(PACK_CURRENT);

  Serial.print("\n--------------------------------------\n");
  
}
