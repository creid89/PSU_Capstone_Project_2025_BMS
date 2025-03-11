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

//Volatile Flag for Charinging status
volatile bool CHARGING = true;
volatile bool BALANCE = false;

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
  BALANCE = true;

}


void setup() {
  // Initialize USB Serial for debugging
  delay(5000);
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }

  //Define Pins as outputs to balancer
  //Cell 1 Balance
  pinMode(PB1, OUTPUT);
  //Cell 2 Balance
  pinMode(PB0, OUTPUT);
  //Cell 3 Balance
  pinMode(PA7, OUTPUT);
  //Cell 4 Balance
  pinMode(PA6, OUTPUT);
  // Initialize Serial2 for UART communication
  Serial2.begin(9600);

  //////////////////////////////comment me out on nondefective boards//////////////////////////////////////
  Wire.setSDA(PB11);
  Wire.setSCL(PB10);
  Wire.begin();
  //////////////////////////////comment me out on nondefective boards//////////////////////////////////////

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
  if (ITimer2.attachInterruptInterval(1000000, onTimerISR)) {
    Serial.println("Timer2 started, checking Serial2 every 1000 ms");
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
  if(BALANCE == true && CHARGING == true){
    Balance_Cells();
  }



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
  PACK_CURRENT = (ina260_0x45.readCurrent())/1000.00; //read current in A
  Serial.print("Current Through the pack = ");
  Serial.println(PACK_CURRENT);
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage()/1000.00;
  Serial.print("Voltage of ina260_0x45 = ");
  Serial.println(INA_0x45_VOLTAGE);
  Serial.print("\n--------------------------------------\n");
}

void Balance_Cells(){
  //Grab each cells Voltage
  INA_0x40_VOLTAGE = ina260_0x40.readBusVoltage()/1000.00;
  INA_0x41_VOLTAGE = ina260_0x41.readBusVoltage()/1000.00;
  INA_0x44_VOLTAGE = ina260_0x44.readBusVoltage()/1000.00;
  INA_0x45_VOLTAGE = ina260_0x45.readBusVoltage()/1000.00;

  //Calulate Cell Voltages based off of INA readings
  CELL1_VOLTAGE = INA_0x45_VOLTAGE - INA_0x44_VOLTAGE;
  CELL2_VOLTAGE = INA_0x44_VOLTAGE - INA_0x41_VOLTAGE;
  CELL3_VOLTAGE = INA_0x41_VOLTAGE - INA_0x40_VOLTAGE;
  CELL4_VOLTAGE = INA_0x40_VOLTAGE;
  //If cell 1 outside of 3.7V max Activate Blancer on Cell 1
  if(CELL1_VOLTAGE >= 3.7)
  {
    Serial.print("CELL 1 OUT OF VOLTAGE RANGE, BYPASSING\n");
    digitalWrite(PB1, HIGH);
    Serial.print("CELL 1 Voltage: ");
    Serial.println(CELL1_VOLTAGE);
  }
  else{
    Serial.print("Cell 1 in balance\n");
    digitalWrite(PB1, LOW);
    Serial.print("CELL 1 Voltage: ");
    Serial.println(CELL1_VOLTAGE);
  }
  //If cell 2 outside of 3.7V max Activate Blancer on Cell 2
  if(CELL2_VOLTAGE >= 3.7)
  {
    Serial.print("CELL 2 OUT OF VOLTAGE RANGE, BYPASSING\n");
    digitalWrite(PB0, HIGH);
    Serial.print("CELL 2 Voltage: ");
    Serial.println(CELL2_VOLTAGE);
  }
  else{
    Serial.print("Cell 2 in balance\n");
    digitalWrite(PB0, LOW);
    Serial.print("CELL 2 Voltage: ");
    Serial.println(CELL2_VOLTAGE);
  }
  //If cell 3 outside of 3.7V max Activate Blancer on Cell 3
  if(CELL3_VOLTAGE >= 3.7)
  {
    Serial.print("CELL 3 OUT OF VOLTAGE RANGE, BYPASSING\n");
    digitalWrite(PA7, HIGH);
    Serial.print("CELL 3 Voltage: ");
    Serial.println(CELL3_VOLTAGE);

  }
  else{
    Serial.print("Cell 3 in balance\n");
    digitalWrite(PA7, LOW);
    Serial.print("CELL 3 Voltage: ");
    Serial.println(CELL3_VOLTAGE);
  }
  //If cell 4 outside of 3.7V max Activate Blancer on Cell 4
  if(CELL4_VOLTAGE >= 3.7)
  {
    Serial.print("CELL 4 OUT OF VOLTAGE RANGE, BYPASSING\n");
    digitalWrite(PA6, HIGH);
    Serial.print("CELL 4 Voltage: ");
    Serial.println(CELL4_VOLTAGE);
  }
  else{
    Serial.print("Cell 4 in balance\n");
    digitalWrite(PA6, LOW);
    Serial.print("CELL 4 Voltage: ");
    Serial.println(CELL4_VOLTAGE);
  }
}