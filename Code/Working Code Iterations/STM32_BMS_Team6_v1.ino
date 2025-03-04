#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>

//need to update the code to update global variable and then use an interrupt to check those global variables so that we know when to start cell balancing

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

// Create an INA260 object
Adafruit_INA260 ina260;

// Volatile flag set by the timer ISR to indicate that it is time to check Serial2
volatile bool serialCheckFlag = false;
volatile bool readINA_1_Flag = false;

// Create a timer instance using TIM2 (adjust if needed)
STM32Timer ITimer2(TIM2);

// Timer interrupt service routine (ISR)
void onTimerISR() {
  serialCheckFlag = true;
  readINA_1_Flag = true;
}


void setup() {
  // Initialize USB Serial for debugging
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }
  
  // Initialize Serial2 for UART communication
  Serial2.begin(9600);
  //Init INA260
  if (!ina260.begin()) {
    Serial.println("Error: INA260 sensor not found!");
    while (1) {
      delay(10);
    }
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
  
  if (readINA_1_Flag) {
    readINA_1_Flag = false;  // Clear the flag
    Serial.println("Timer ISR triggered, reading sensor...");
    readINA_1_values();
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

void readINA_1_values() {
  float current_mA = ina260.readCurrent();     // Current in mA
  float voltage_V  = ina260.readBusVoltage();    // Voltage in V
  float power_mW   = ina260.readPower();         // Power in mW

  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.print(" mA, Voltage: ");
  Serial.print(voltage_V);
  Serial.print(" V, Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");
}
