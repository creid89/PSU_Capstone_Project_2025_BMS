#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_INA260.h>

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

//Create object for INA260
Adafruit_INA260 INA260;

//Sensor Fla

uint8_t receivedByte = 0;
unsigned long previousMillis = 0;
const unsigned long uartCheckInterval = 10;  // Check for UART data every 10 ms

void setup() {
  //Start serial for sending over USB
  Serial.begin(9600); //inti comms over USB
  while(!Serial){
    //wait an extra 10ms for Serial USB to open
  }
  Serial2.begin(9600); //init comms over RX/TX pins for recieiving data

  // Initialize the INA260 sensor (default I2C address 0x40)
  if (!INA260.begin()) {
    Serial.println("Error: INA260 sensor not found!");
    while (1) { 
      delay(10);
    }
  }  


  

}

void loop() {
  // Check periodically (non-blocking) for incoming UART data
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= uartCheckInterval) {
    previousMillis = currentMillis; 
    readUARTbus();
  }
    
      /*Serial.print("Received Byte: 0x");
      if (receivedByte < 0x10) {
        Serial.print("0");
      }
      Serial.println(receivedByte, HEX);
    }*/
  

    
  
  delay(100);
}

void readUARTbus(){
// Process all available bytes on Serial2
    while (Serial2.available() > 0) {
      uint8_t receivedByte = Serial2.read();
      switch(receivedByte){
        case 0x01:
          
          Serial.print("Requested Voltage of Cell 1\n");
          //call function that measures voltage
          break;
        case 0x02:
          Serial.print("Requested Voltage of Cell 2\n");
          //call function that measures voltage
          break;
        case 0x03:
          Serial.print("Requested Voltage of Cell 3\n");
          //call function that measures voltage
          break;
        case 0x04:
          Serial.print("Requested Voltage of Cell 4\n");
          //call function that measures voltage
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
