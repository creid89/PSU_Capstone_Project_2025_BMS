// Include necessary libraries
#include <HardwareSerial.h>

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

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


  

}

void loop() {
  //serialEvent2();
  // Check periodically (non-blocking) for incoming UART data
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= uartCheckInterval) {
    previousMillis = currentMillis;
    
    // Process all available bytes on Serial2
    while (Serial2.available() > 0) {
      uint8_t receivedByte = Serial2.read();
      Serial.print("Received Byte: 0x");
      if (receivedByte < 0x10) {
        Serial.print("0");
      }
      Serial.println(receivedByte, HEX);
    }
  }


  delay(100);
}

//This function automatically get called when new data arrives on Serial2
void serialEvent2(){
  delay(500);
  Serial.println("Debug Line 27\n");
  while (Serial2.available() > 0){
    receivedByte = Serial2.read();
    Serial.print("Byte Recieved: ");
    Serial.println(receivedByte, HEX);
    //Serial2.flush();
    receivedByte = 0;
  }
  delay(500);
  Serial.println("Debug Line 36\n");

}