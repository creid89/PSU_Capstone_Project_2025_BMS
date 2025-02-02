#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#define I2C_PORT i2c1 //using i2c1 for comms on current sensor and ADC IC
#define SDA_PIN 4  //GPIO6 / D4 / SDA1
#define SCL_PIN 5 //GPIO7 / D5 / SCL1
#define I2C_FREQ 100000 //100kHz
#define INA219_I2C_ADDR 0x40 //INA219 Default I2C Address
#define ADS7830_I2C_ADDR 0x48 //ADS7830 Base Address
#define I2C_SLAVE_ADDR 0x08 //to be used with slave init of RP2350

#define LED_PIN 25

// Initialize the GPIO for the LED
void pico_led_init(void) {
#ifdef PICO_DEFAULT_LED_PIN
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

// Turn the LED on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#endif
}

void init_i2c() {

    i2c_init(I2C_PORT, I2C_FREQ);
    //i2c_set_slave_mode(i2c0, true, 0x08);

    //init GPIO pins as i2c
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    printf("\nInit in slave mode\n");
    i2c_set_slave_mode(I2C_PORT, true, I2C_SLAVE_ADDR );
    printf("");


}

void send_i2c_data(uint8_t data, uint8_t addr){
    printf("\n In send_i2c_data func\n");
    uint8_t buffer[1] = {data};

    int result = i2c_write_blocking(I2C_PORT, addr, buffer, 1, true);
    
    printf("\nAttempted to send data to 0x%02X\n", addr);

    if(result == PICO_ERROR_GENERIC){
        printf("\nI2C Write Failed!\n");
    }else{
        printf("I2C Data Sent: 0x%02X\n", data);
    }

}





int main() {
    stdio_init_all();
    sleep_ms(2000); // wait for USB connection
    pico_led_init();
    init_i2c();

    //send_i2c_data(0xFF, INA219_I2C_ADDR);

    

    while(1){
        pico_set_led(true);
        printf("LED ON\n");
        sleep_ms(2500);
        pico_set_led(false);
        printf("LED OFF\n");
        sleep_ms(2500);
    }

}