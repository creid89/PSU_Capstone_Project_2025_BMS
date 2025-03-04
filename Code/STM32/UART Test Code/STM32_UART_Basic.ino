// Include necessary libraries
#include <HardwareSerial.h>

// Define Serial2 on PA2 (TX2) and PA3 (RX2)
HardwareSerial Serial2(PA3, PA2);  // RX, TX

uint8_t receivedByte = 0;

void setup() {
  //Start serial for sending over USB
  Serial.begin(9600); //inti comms over USB
  Serial2.begin(9600); //init comms over RX/TX pins for recieiving data
  

}

void loop() {
  delay(500);
  //Serial.print("In loop() at Line 13 above while (Serial1.available() > 0) loop\n");
  while (Serial2.available() > 0){
    receivedByte = Serial2.read();
    Serial.print("Byte Recieved: \n");
    Serial.print(receivedByte, HEX);
    //Serial2.flush();
    receivedByte = 0;
  }
  delay(500);
  //Serial.print("In loop() at Line 20 below while (Serial1.available() > 0) loop\n");


}
