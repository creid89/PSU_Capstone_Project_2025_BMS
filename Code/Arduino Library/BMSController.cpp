#include "BMSController.h"
float SoCGiven;

BMSController::BMSController(uint8_t i2cAddress) {
  _address = i2cAddress;
  Wire.begin();
}

bool BMSController::begin() {
  Wire.begin();
  Wire.beginTransmission(_address);
  if (Wire.endTransmission() == 0) {
    return true;  // Device responded
  }
  return false;  // No response
}

/**void BMSController::sendFloat(uint8_t command, float value) {
  Wire.beginTransmission(_address);
  Wire.write(command);

  uint8_t buf[4];
  memcpy(buf, &value, sizeof(float));
  Wire.write(buf, 4);

  Wire.endTransmission();
}**/
///////////////////////////////////EXPIREMENTAL/////////////////////////////////////////////////
void BMSController::sendFloat(uint8_t command, float value) {
  const uint8_t maxRetries = 10;
  const uint16_t retryDelayMs = 5;

  uint8_t retries = 0;
  int transmissionResult = 4; // Arbitrary non-zero value to enter loop

  // Retry loop to wait for the bus to become available
  while (retries < maxRetries) {
    Wire.beginTransmission(_address);
    Wire.write(command);

    uint8_t buf[4];
    memcpy(buf, &value, sizeof(float));
    Wire.write(buf, 4);

    transmissionResult = Wire.endTransmission();
    if (transmissionResult == 0) {
      // Success
      break;
    } else {
      // Transmission failed, possibly bus busy or NACKed
      retries++;
      delay(retryDelayMs);
    }
  }

  if (transmissionResult != 0) {
    // Optional: handle error here (e.g., log or raise flag)
    Serial.print("I2C transmission failed, error code: ");
    Serial.println(transmissionResult);
  }
}////////////////////////////////////////////////////////////////////////////////

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

bool BMSController::setCellMaxCutOffV(float volts) {
  sendFloat(0x16, volts);
  return true;
}

bool BMSController::setCellMinCutOffV(float volts) {
  sendFloat(0x17, volts);
  return true;
}

bool BMSController::setPack_stock_capacity(float mAh) {
SoCGiven = mAh;
  sendFloat(0x18, mAh);
  return true;
}
bool BMSController::FinishedConfiguration(bool completed) {
  if(completed == true)
  {
	  sendFloat(0x19, completed);
	  return true;
	  
  }
  return false;
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

float BMSController::getSoC() {
  return getValue(0x08);
}

float BMSController::getPercentSoC() {
  float SoC = getValue(0x08);
  float PercentSoC = SoC/SoCGiven*100;
  return PercentSoC;
  
}