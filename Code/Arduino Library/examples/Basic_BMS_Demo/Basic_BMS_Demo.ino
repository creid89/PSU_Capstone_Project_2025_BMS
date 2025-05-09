#include <BMSController.h>

#define SDA_PIN 23
#define SCL_PIN 22
#define BMS_ADDR 0x58

BMSController BMS(BMS_ADDR, SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(115200);
  BMS.begin();

  BMS.setChargeVoltage(16.8);
  BMS.setVsysMin(8.0);
  BMS.setInputVoltageLimit(12.5);
  BMS.setChargeCurrent(1.5);
  BMS.setInputCurrentLimit(3000);

  float chargeCurrent = BMS.getValue(0x06);
  Serial.print("Charge Current: ");
  Serial.println(chargeCurrent, 3);
}

void loop() {
  // Add periodic reads or config here
}