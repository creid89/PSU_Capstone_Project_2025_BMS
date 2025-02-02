#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define LED_PIN 25 // Adjust according to your board's LED pin
#define I2C_ADDR 0x04 // I2C address of this device

void i2c_slave_handler();

int main() {
    // Initialize standard I/O for serial communication
    stdio_init_all();

    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    Serial.begin(9600); 
    // Initialize I2C in slave mode
    i2c_init(I2C_PORT, 100 * 1000); // I2C at 100 kHz
    gpio_set_function(4, GPIO_FUNC_I2C); // SDA pin (GPIO4)
    gpio_set_function(5, GPIO_FUNC_I2C); // SCL pin (GPIO5)
    gpio_pull_up(4);
    gpio_pull_up(5);

    // Set the I2C slave address
    i2c_set_slave_mode(I2C_PORT, true, I2C_ADDR);

    while (1) {
        i2c_slave_handler(); // Handle I2C communication
        tight_loop_contents();
    }
}

void i2c_slave_handler() {
    // Check if data is available in the I2C RX FIFO
    int rx_fifo_level = i2c_get_read_available(I2C_PORT);

    if (rx_fifo_level > 0) {
        uint8_t received_data;
        // Read data from the I2C bus
        i2c_read_raw_blocking(I2C_PORT, &received_data, 1);

        // Output to serial monitor
        printf("Received I2C data: %d\n", received_data);

        if (received_data == 1) {
            // Turn the LED on and off
            gpio_put(LED_PIN, 1);
            sleep_ms(500);
            gpio_put(LED_PIN, 0);

            // Print confirmation
            printf("Blink command executed.\n");
        }
    }
}
