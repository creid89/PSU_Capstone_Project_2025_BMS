#ifndef BMSController_h
#define BMSController_h

#include <Arduino.h>
#include <Wire.h>

class BMSController {
  public:
    BMSController(uint8_t i2cAddress, uint8_t sdaPin, uint8_t sclPin);
    void begin();
    bool setChargeVoltage(float volts);
    bool setVsysMin(float volts);
    bool setInputVoltageLimit(float volts);
    bool setChargeCurrent(float amps);
    bool setInputCurrentLimit(float milliamps);
    float getValue(uint8_t command);
    
  private:
    uint8_t _address;
    void sendFloat(uint8_t command, float value);
};

#endif