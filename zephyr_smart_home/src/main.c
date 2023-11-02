#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <pthread.h>

#include "../inc/lcd_screen_i2c.h"
#include "../inc/adc_handler.h"

#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LCD_1602 DT_ALIAS(lcd_screen)
#define BUTTON_0 DT_ALIAS(button_0)
#define BUTTON_1 DT_ALIAS(button_1)
#define BUZZ_0 DT_ALIAS(buzz_0)

const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
const struct i2c_dt_spec dev_lcd_screen = I2C_DT_SPEC_GET(LCD_1602);

const struct gpio_dt_spec b0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
const struct gpio_dt_spec b1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});

const struct gpio_dt_spec buzz0 = GPIO_DT_SPEC_GET_OR(BUZZ_0, gpios, {0});

static struct gpio_callback b0_callback;
static struct gpio_callback b1_callback;
volatile int flag = 0;

K_SEM_DEFINE(init_gpio_sem,0,2);

void button0_pressed(const struct device *dev, struct gpio_callback *cb,uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	flag = 1;
}

void button1_pressed(const struct device *dev, struct gpio_callback *cb,uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	flag = 0;
}

void sensor_channel_thread() {
	k_sem_take(&init_gpio_sem, K_FOREVER);
	init_adc_driver();
 
	while(1) {

		struct sensor_value temp, press, humidity;

		sensor_sample_fetch(dht11);
		sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dht11, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);

		printk("temp: %d.%06d;  humidity: %d.%06d ",
		      temp.val1, temp.val2, 
		      humidity.val1, humidity.val2);
		
		int val  = read_analog_data();
		
		printk("val steam: %d\n", val);

		k_sleep(K_MSEC(1000));
	}
}

void button_thread() {

	k_sem_take(&init_gpio_sem, K_FOREVER);

	while (1) {
		lcd_alarm_on(flag, &dev_lcd_screen, START_ALERT_MONITORING_MSG_1, LCD_LINE_2 );
		lcd_alarm_off(flag, &dev_lcd_screen, STOP_ALERT_MONITORING_MSG_2, LCD_LINE_2 );
	}
}


int main(void) {

    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);
	
    // Init device
    init_lcd(&dev_lcd_screen);

    // Display a message
    write_lcd(&dev_lcd_screen, HELLO_MSG, LCD_LINE_1);
    //write_lcd(&dev_lcd_screen, ZEPHYR_MSG, LCD_LINE_2);

	// Button configuration
    gpio_pin_configure_dt(&b0, GPIO_INPUT);
    gpio_pin_configure_dt(&b1, GPIO_INPUT);

	//gpio_pin_configure_dt(&buzz0, GPIO_OUTPUT_LOW);

	int val0 = gpio_pin_get_dt(&b0);
	int val1 = gpio_pin_get_dt(&b1);

	gpio_pin_interrupt_configure_dt(&b0, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&b1, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&b0_callback, button0_pressed, BIT(b0.pin));
	gpio_add_callback(b0.port, &b0_callback);
		
	gpio_init_callback(&b1_callback, button1_pressed, BIT(b1.pin));
	gpio_add_callback(b1.port, &b1_callback);

	k_sem_give(&init_gpio_sem);
	k_sem_give(&init_gpio_sem);

	while(1){
		gpio_pin_configure_dt(&buzz0, GPIO_OUTPUT_HIGH);
		k_sleep(K_MSEC(1));
		gpio_pin_configure_dt(&buzz0, GPIO_OUTPUT_LOW);
	}

	return 0;
}

K_THREAD_DEFINE(sensor_channel_thread_id, 521, sensor_channel_thread, NULL, NULL, NULL, 9, 0, 0);
//K_THREAD_DEFINE(steam_thread_id, 521, steam_thread, NULL, NULL, NULL, 9, 0, 0);
K_THREAD_DEFINE(button_thread_id, 521, button_thread, NULL, NULL, NULL, 9, 0, 0);
//K_THREAD_DEFINE(buzzer_thread_id, 521, buzzer_thread, NULL, NULL, NULL, 9, 0, 0);
