#include <BMSController.h>

//Using Huzzah Adafruit ESP32
#define SDA_PIN 23
#define SCL_PIN 22
#define BMS_ADDR 0x58

BMSController BMS(BMS_ADDR, SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(115200);
  BMS.begin();

  delay(2500);

  BMS.setChargeVoltage(16.8);       // command 0x10
  delay(100);
  BMS.setVsysMin(8.0);              // command 0x11
  delay(100);
  BMS.setInputVoltageLimit(12.5);   // command 0x13
  delay(100);
  BMS.setChargeCurrent(1.5);        // command 0x14
  delay(100);
  BMS.setInputCurrentLimit(3000);   // command 0x15
  delay(100);

}

void loop() {
  delay(1500);
  float chargeCurrent = BMS.getChargeCurrent();
  Serial.print("Charge Current: ");
  Serial.println(chargeCurrent, 3);
   delay(1500);
  float cellVoltage1 = BMS.getCell1Voltage();
  Serial.print("Cell 1 Voltage: ");
  Serial.println(cellVoltage1 , 3);
  delay(1500);
  float cellVoltage2 = BMS.getCell2Voltage();
  Serial.print("Cell 2 Voltage: ");
  Serial.println(cellVoltage2 , 3);
  delay(1500);
  float cellVoltage3 = BMS.getCell3Voltage();
  Serial.print("Cell 3 Voltage: ");
  Serial.println(cellVoltage3 , 3);
  delay(1500);
  float cellVoltage4 = BMS.getCell4Voltage();
  Serial.print("Cell 4 Voltage: ");
  Serial.println(cellVoltage4 , 3);
  delay(1500);
  float totalPackVoltage = BMS.getTotalPackVoltage();
  Serial.print("Total Pack Voltage: ");
  Serial.println(totalPackVoltage , 3);
  delay(1500);
}
