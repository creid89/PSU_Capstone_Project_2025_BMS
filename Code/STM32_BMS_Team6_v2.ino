#include <Wire.h>
#include <Adafruit_INA260.h>
#include <HardwareSerial.h>
#include <STM32TimerInterrupt.h>

//need to update the code to update global variable and then use an interrupt to check those global variables so that we know when to start cell balancing
// Initialize I2C2 on PB11 (SDA) and PB10 (SCL)


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

// Create a timer instance using TIM2 (adjust if needed)
STM32Timer ITimer2(TIM2);

// Timer interrupt service routine (ISR)
void onTimerISR() {
  serialCheckFlag = true;
  readINA_1_Flag = true;
}


void setup() {
  // Initialize USB Serial for debugging
  delay(5000);
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }
  
  // Initialize Serial2 for UART communication
  Serial2.begin(9600);
  Wire.setSDA(PB11);
  Wire.setSCL(PB10);
  Wire.begin();
  //Init INA260
  // Initialize each INA260 with its corresponding I2C address
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
  if (ITimer2.attachInterruptInterval(2000000, onTimerISR)) {
    Serial.println("Timer2 started, checking Serial2 every 2000 ms");
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
    Serial.println(" ");
    Serial.println("Timer ISR triggered, reading sensor...");
    readINA_1_values();
    readINA_2_values();
    readINA_3_values();
    readINA_4_values();
    Serial.println(" ");
    Serial.println("----------------------------------------------------------------------------");
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
  float current_mA = ina260_0x40.readCurrent();     // Current in mA
  float voltage_V  = ina260_0x40.readBusVoltage();    // Voltage in V
  float power_mW   = ina260_0x40.readPower();         // Power in mW
  Serial.print("INA 1 ");
  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.print(" mA, Voltage: ");
  Serial.print(voltage_V/1000);
  Serial.print(" V, Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");
}
void readINA_2_values() {
  float current_mA = ina260_0x41.readCurrent();     // Current in mA
  float voltage_V  = ina260_0x41.readBusVoltage();    // Voltage in V
  float power_mW   = ina260_0x41.readPower();         // Power in mW
  Serial.print("INA 2 ");
  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.print(" mA, Voltage: ");
  Serial.print(voltage_V/1000);
  Serial.print(" V, Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");
}
void readINA_3_values() {
  float current_mA = ina260_0x44.readCurrent();     // Current in mA
  float voltage_V  = ina260_0x44.readBusVoltage();    // Voltage in V
  float power_mW   = ina260_0x44.readPower();         // Power in mW
  Serial.print("INA 3 ");
  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.print(" mA, Voltage: ");
  Serial.print(voltage_V/1000);
  Serial.print(" V, Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");
}
void readINA_4_values() {
  float current_mA = ina260_0x45.readCurrent();     // Current in mA
  float voltage_V  = ina260_0x45.readBusVoltage();    // Voltage in V
  float power_mW   = ina260_0x45.readPower();         // Power in mW
  Serial.print("INA 4 ");
  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.print(" mA, Voltage: ");
  Serial.print(voltage_V/1000);
  Serial.print(" V, Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");
}
