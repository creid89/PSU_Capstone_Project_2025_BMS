#ifndef BMSController_h
#define BMSController_h

#include <Arduino.h>
#include <Wire.h>

class BMSController {
  public:
    BMSController(uint8_t i2cAddress);
    bool begin();
    bool setChargeVoltage(float volts);
    bool setVsysMin(float volts);
    bool setInputVoltageLimit(float volts);
    bool setChargeCurrent(float amps);
    bool setInputCurrentLimit(float milliamps);
	bool setCellMaxCutOffV(float volts);
	bool setCellMinCutOffV(float volts);
	bool setPack_stock_capacity(float mAh);
	bool FinishedConfiguration(bool completed);

    float getChargeCurrent();
    float getCell1Voltage();
    float getCell2Voltage();
    float getCell3Voltage();
    float getCell4Voltage();
    float getTotalPackVoltage();

  private:
    uint8_t _address;
    void sendFloat(uint8_t command, float value);
    float getValue(uint8_t command);
};

#endif