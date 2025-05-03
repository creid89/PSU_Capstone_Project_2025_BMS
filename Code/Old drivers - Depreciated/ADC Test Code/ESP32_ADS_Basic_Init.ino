#include <Wire.h>
#include <Adafruit_ADS7830.h>

Adafruit_ADS7830 ADC;
#define V_REF 3.000  // Reference voltage
unsigned long lastUpdate = 0;
const int SENSOR_UPDATE_INTERVAL = 1000; // 1 second

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("ADS7830 Module Test");

  if (!ADC.begin()) {
    Serial.println("Failed to initialize ADS7830");
    while (1);
  }
}

void loop() {
  if (millis() - lastUpdate >= SENSOR_UPDATE_INTERVAL) {
    lastUpdate = millis();
    read_battery_cell_0();
    read_battery_cell_1();
    read_battery_cell_2();
    read_battery_cell_3();
    read_battery_pack_4s();
  }
}

void read_battery_cell_0() {
  uint8_t RAW_ADC_CHAN_0 = ADC.readADCsingle(0);
  uint8_t RAW_ADC_CHAN_1 = ADC.readADCsingle(1);

  float VOLT_ADC_CHAN_0 = (RAW_ADC_CHAN_0 / 255.0) * V_REF;
  float VOLT_ADC_CHAN_1 = (RAW_ADC_CHAN_1 / 255.0) * V_REF;
  float battery_cell_0_voltage = VOLT_ADC_CHAN_0 - VOLT_ADC_CHAN_1;

  Serial.print("Cell 0 ADC Value: ");
  Serial.println(battery_cell_0_voltage, 3);
}

void read_battery_cell_1() {
  uint8_t RAW_ADC_CHAN_1 = ADC.readADCsingle(1);
  uint8_t RAW_ADC_CHAN_2 = ADC.readADCsingle(2);

  float VOLT_ADC_CHAN_1 = (RAW_ADC_CHAN_1 / 255.0) * V_REF;
  float VOLT_ADC_CHAN_2 = (RAW_ADC_CHAN_2 / 255.0) * V_REF;
  float battery_cell_1_voltage = VOLT_ADC_CHAN_1 - VOLT_ADC_CHAN_2;

  Serial.print("Cell 1 ADC Value: ");
  Serial.println(battery_cell_1_voltage, 3);
}

void read_battery_cell_2() {
  uint8_t RAW_ADC_CHAN_2 = ADC.readADCsingle(2);
  uint8_t RAW_ADC_CHAN_3 = ADC.readADCsingle(3);

  float VOLT_ADC_CHAN_2 = (RAW_ADC_CHAN_2 / 255.0) * V_REF;
  float VOLT_ADC_CHAN_3 = (RAW_ADC_CHAN_3 / 255.0) * V_REF;
  
  float battery_cell_2_voltage = VOLT_ADC_CHAN_2 - VOLT_ADC_CHAN_3;

  Serial.print("Cell 2 ADC Value: ");
  Serial.println(battery_cell_2_voltage, 3);
}

void read_battery_cell_3() {
  uint8_t RAW_ADC_CHAN_3 = ADC.readADCsingle(3);
  uint8_t RAW_ADC_CHAN_4 = ADC.readADCsingle(4);

  float VOLT_ADC_CHAN_3 = (RAW_ADC_CHAN_3 / 255.0) * V_REF;
  float VOLT_ADC_CHAN_4 = (RAW_ADC_CHAN_4 / 255.0) * V_REF;
  
  float battery_cell_3_voltage = VOLT_ADC_CHAN_3 - VOLT_ADC_CHAN_4;

  Serial.print("Cell 3 ADC Value: ");
  Serial.println(battery_cell_3_voltage, 3);
}

void read_battery_pack_4s() {
  uint8_t RAW_ADC_CHAN_0 = ADC.readADCsingle(0);
  uint8_t RAW_ADC_CHAN_4 = ADC.readADCsingle(4);

  float VOLT_ADC_CHAN_0 = (RAW_ADC_CHAN_0 / 255.0) * V_REF;
  float VOLT_ADC_CHAN_4 = (RAW_ADC_CHAN_4 / 255.0) * V_REF;
  
  float battery_pack_voltage = VOLT_ADC_CHAN_0 - VOLT_ADC_CHAN_4;

  Serial.print("Battery Pack 4s ADC Value: ");
  Serial.println(battery_pack_voltage, 3);
}
