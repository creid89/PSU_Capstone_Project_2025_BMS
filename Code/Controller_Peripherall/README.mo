/*
Working on casting the cell voltage value from a floating point to a hex value to send across i2c
The data being recieved on the controller side is not converting back correctly
Could be an issue with unint8_t vs uint16_t delcrations in both programs
Need to "un-cast" the value on the controller side
This version has intergrated the i2c2 bus
The serial2 bus for comms over UART is left in
the v5 of the code on github the cell balancing but this version does not

This file contains both the controller (ESP32) and the peripheral (STM32)

*/
