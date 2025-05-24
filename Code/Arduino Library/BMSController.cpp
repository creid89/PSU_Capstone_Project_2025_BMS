#include "BMSController.h"

BMSController::BMSController(uint8_t i2cAddress) {
  _address = i2cAddress;
  Wire.begin();
}

void BMSController::begin() {
  // Initialize if needed
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

float BMSController::getChargeCurrent() {
  return getValue(0x06);
}

float BMSController::getCell1Voltage() {
  return getValue(0x01);
}

float BMSController::getCell2Voltage() {
  return getValue(0x02);
}

float BMSController::getCell3Voltage() {
  return getValue(0x03);
}

float BMSController::getCell4Voltage() {
  return getValue(0x04);
}

float BMSController::getTotalPackVoltage() {
  return getValue(0x05);
}