#include <Wire.h>
#include <Adafruit_INA260.h>
#include <STM32TimerInterrupt.h>

// Create an INA260 object
Adafruit_INA260 ina260;

// Create a timer instance for TIM2 (adjust if needed)
STM32Timer ITimer2(TIM2);

// Volatile flag set by the timer ISR to indicate a sensor reading is due
volatile bool readSensorFlag = false;

// Timer interrupt service routine (ISR)
// Keep this ISR very short: only set a flag.
void onTimerISR() {
  readSensorFlag = true;
}

// Function to read INA260 sensor values and print them
void readSensorValues() {
  float current_mA = ina260.readCurrent();     // Current in mA
  float voltage_V  = ina260.readBusVoltage();    // Voltage in V
  float power_mW   = ina260.readPower();         // Power in mW

  Serial.print("Current: ");
  Serial.print(current_mA);
  Serial.print(" mA, Voltage: ");
  Serial.print(voltage_V);
  Serial.print(" V, Power: ");
  Serial.print(power_mW);
  Serial.println(" mW");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor to connect
  }
  Serial.println("Initializing INA260 sensor...");
  
  // Initialize the INA260 sensor (default I2C address is 0x40)
  if (!ina260.begin()) {
    Serial.println("Error: INA260 sensor not found!");
    while (1) {
      delay(10);
    }
  }
  Serial.println("INA260 sensor initialized successfully!");

  // Setup timer interrupt: 500,000 microseconds = 500 ms
  if (ITimer2.attachInterruptInterval(500000, onTimerISR)) {
    Serial.println("Timer2 started, sensor readings every 500ms");
  } else {
    Serial.println("Failed to start Timer2");
  }
}

void loop() {
  static unsigned long lastHeartbeat = 0;

  // If the timer ISR set the flag, process the sensor reading
  if (readSensorFlag) {
    readSensorFlag = false;  // Clear the flag
    Serial.println("Timer ISR triggered, reading sensor...");
    readSensorValues();
  }
  
  // Optional: Print a heartbeat every 5 seconds to show the main loop is active.
  if (millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    Serial.println("Main loop heartbeat...");
  }
  
  // Other tasks can be processed here concurrently.
}
