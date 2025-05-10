#include <BMSController.h>

#define SDA_PIN 23
#define SCL_PIN 22
#define BMS_ADDR 0x58

BMSController BMS(BMS_ADDR, SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(115200);
  BMS.begin();

  delay(2500);

  BMS.setChargeVoltage(16.8);
  BMS.setVsysMin(8.0);
  BMS.setInputVoltageLimit(12.5);
  BMS.setChargeCurrent(1.5);
  BMS.setInputCurrentLimit(3000);
}

void loop() {
  Serial.print("Charge Current: ");
  Serial.println(BMS.getChargeCurrent(), 3);

  Serial.print("Cell 1 Voltage: ");
  Serial.println(BMS.getCell1Voltage(), 3);

  Serial.print("Cell 2 Voltage: ");
  Serial.println(BMS.getCell2Voltage(), 3);

  Serial.print("Cell 3 Voltage: ");
  Serial.println(BMS.getCell3Voltage(), 3);

  Serial.print("Cell 4 Voltage: ");
  Serial.println(BMS.getCell4Voltage(), 3);

  Serial.print("Total Pack Voltage: ");
  Serial.println(BMS.getTotalPackVoltage(), 3);

  delay(3000);
}