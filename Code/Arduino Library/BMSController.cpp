#include "BMSController.h"

BMSController::BMSController(uint8_t i2cAddress, uint8_t sdaPin, uint8_t sclPin) {
  _address = i2cAddress;
  Wire.begin(sdaPin, sclPin);
}

void BMSController::begin() {
  // Optionally reserve I2C buffers or do nothing
}

void BMSController::sendFloat(uint8_t command, float value) {
  Wire.beginTransmission(_address);
  Wire.write(command);

  uint8_t buf[4];
  memcpy(buf, &value, sizeof(float));
  Wire.write(buf, 4);

  Wire.endTransmission();
}

bool BMSController::setChargeVoltage(float volts) {
  sendFloat(0x10, volts);
  return true;
}

bool BMSController::setVsysMin(float volts) {
  sendFloat(0x11, volts);
  return true;
}

bool BMSController::setInputVoltageLimit(float volts) {
  sendFloat(0x13, volts);
  return true;
}

bool BMSController::setChargeCurrent(float amps) {
  sendFloat(0x14, amps);
  return true;
}

bool BMSController::setInputCurrentLimit(float milliamps) {
  sendFloat(0x15, milliamps);
  return true;
}

float BMSController::getValue(uint8_t command) {
  float value = -1.0;

  Wire.beginTransmission(_address);
  Wire.write(command);
  if (Wire.endTransmission() != 0) return -1.0;

  Wire.requestFrom(_address, (uint8_t)4);
  if (Wire.available() < 4) return -1.0;

  uint8_t buf[4];
  for (int i = 0; i < 4; i++) buf[i] = Wire.read();
  memcpy(&value, buf, 4);

  return value;
}