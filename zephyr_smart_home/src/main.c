#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>



#include "../inc/lcd_screen_i2c.h"

#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LCD_1602 DT_ALIAS(lcd_screen)


const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
const struct i2c_dt_spec dev_lcd_screen = I2C_DT_SPEC_GET(LCD_1602);

int main(void) {
    
    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);

    // Init device
    init_lcd(&dev_lcd_screen);

    // Display a message
    write_lcd(&dev_lcd_screen, HELLO_MSG, LCD_LINE_1);
    write_lcd(&dev_lcd_screen, ZEPHYR_MSG, LCD_LINE_2);

    while (1) {
		struct sensor_value temp, press, humidity;

		sensor_sample_fetch(dht11);
		sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dht11, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);

		printk("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
		      temp.val1, temp.val2, press.val1, press.val2,
		      humidity.val1, humidity.val2);

		k_sleep(K_MSEC(1000));
	}
	return 0;
}

