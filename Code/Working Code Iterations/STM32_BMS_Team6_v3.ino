#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>

//need to update the code to update global variable and then use an interrupt to check those global variables so that we know when to start cell balancing

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

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
}



// Function to read and print data from Serial2
void readSerial2Data() {
  while (Serial2.available() > 0) {
    uint8_t receivedByte = Serial2.read();
    Serial.print("Byte Received: ");
    Serial.println(receivedByte, HEX);
  }
}

void read_INA260_0x40_values() {
  //float current_mA = ina260_0x40.readCurrent();     // Current in mA
  //float voltage_V  = ina260_0x40.readBusVoltage();    // Voltage in V
  //float power_mW   = ina260_0x40.readPower();         // Power in mW
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage()/1000.00;
  Serial.print("\n Voltage of ina260_0x40 = ");
  Serial.println(INA_0x40_VOLTAGE); 


}

void read_INA260_0x41_values() {
  //float current_mA = ina260_0x41.readCurrent();     // Current in mA
  //float voltage_V  = ina260_0x41.readBusVoltage();    // Voltage in V
  //float power_mW   = ina260_0x41.readPower();         // Power in mW
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage()/1000.00;
  Serial.print("\n Voltage of ina260_0x41 = ");
  Serial.println(INA_0x41_VOLTAGE);  

}

void read_INA260_0x44_values() {
  //float current_mA = ina260_0x44.readCurrent();     // Current in mA
  //float voltage_V  = ina260_0x44.readBusVoltage();    // Voltage in V
  //float power_mW   = ina260_0x44.readPower();         // Power in mW
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage()/1000.00;
  Serial.print("Voltage of ina260_0x44 = ");
  Serial.println(INA_0x44_VOLTAGE);
}

void read_INA260_0x45_values() {
  //this function is used measure current flowing into the pack
  //This will constantly update the global float 
  PACK_CURRENT = (ina260_0x45.readCurrent())/1000.00; //read current in A
  Serial.print("Current Through the pack = ");
  Serial.println(PACK_CURRENT);
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage()/1000.00;
  Serial.print("Voltage of ina260_0x45 = ");
  Serial.println(INA_0x45_VOLTAGE);
  
}
