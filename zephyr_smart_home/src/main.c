#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/http/client.h>
#include <errno.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>


#include "../inc/lcd_screen_i2c.h"
#include "../inc/adc_handler.h"
#include "../inc/wifi_handler.h"

#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LCD_1602 DT_ALIAS(lcd_screen)
#define BUTTON_0 DT_ALIAS(button_0)
#define BUTTON_1 DT_ALIAS(button_1)
#define BUZZ_0 DT_ALIAS(buzz_0)
#define MOTION_SENSOR DT_ALIAS(motion_sensor)

K_MUTEX_DEFINE(lcd_mutex);

const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
const struct i2c_dt_spec dev_lcd_screen = I2C_DT_SPEC_GET(LCD_1602);

const struct gpio_dt_spec b0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
const struct gpio_dt_spec b1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});

const struct gpio_dt_spec buzz0 = GPIO_DT_SPEC_GET_OR(BUZZ_0, gpios, {0});

const struct gpio_dt_spec MotionSensor = GPIO_DT_SPEC_GET_OR(MOTION_SENSOR, gpios, {0});

static struct gpio_callback b0_callback;
static struct gpio_callback b1_callback;
volatile int flag = 0;
volatile int flag_d = 0;
volatile int state_alarm = 0;
volatile int state_intru = 0;


K_SEM_DEFINE(init_gpio_sem,0,3);

//K_SEM_DEFINE(alarm_sem,0,1);


void button0_pressed(const struct device *dev, struct gpio_callback *cb,uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	flag = 1;
	printk("flag int %d\n",flag);
}

void button1_pressed(const struct device *dev, struct gpio_callback *cb,uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	flag_d = 1;
	printk("flag_d int %d\n",flag);
}

void sensor_channel_thread() {

	k_sem_take(&init_gpio_sem, K_FOREVER);

	printk("Je suis dans le thread sensor\n");
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

/*
void button_thread() {

	k_sem_take(&init_gpio_sem, K_FOREVER);
}
*/

//lcd_alarm_off(flag, &dev_lcd_screen, STOP_ALERT_MONITORING_MSG_2, LCD_LINE_2 );
//lcd_alarm_on(flag_d, &dev_lcd_screen, START_ALERT_MONITORING_MSG_1, LCD_LINE_2 );

void alarm_thread(){

	k_sem_take(&init_gpio_sem, K_FOREVER);
	printk("Je suis dans le thread alarm lcd\n");

	while(1) 
	{

		int sens_val = gpio_pin_get_dt(&MotionSensor);


		if(sens_val == 0 && state_alarm == 1 && flag == 0  && flag_d == 0) {
			k_mutex_lock(&lcd_mutex, K_FOREVER);
			write_lcd(&dev_lcd_screen, INTRUDER_MSG_1, LCD_LINE_1);
			write_lcd(&dev_lcd_screen, INTRUDER_MSG_2, LCD_LINE_2);
			k_mutex_unlock(&lcd_mutex);

		}
		else if(sens_val == 1 && state_alarm == 1 && flag == 0  && flag_d == 0) {
			k_mutex_lock(&lcd_mutex, K_FOREVER);
			write_lcd(&dev_lcd_screen, HELLO_MSG, LCD_LINE_1);
			write_lcd(&dev_lcd_screen, START_ALERT_MONITORING_MSG_1, LCD_LINE_2);
			k_mutex_unlock(&lcd_mutex);
		}

		k_sleep(K_MSEC(10));

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

	// Motion sensor conf
	gpio_pin_configure_dt(&MotionSensor, GPIO_INPUT);
	gpio_pin_configure_dt(&buzz0, GPIO_OUTPUT_LOW);

	int val0 = gpio_pin_get_dt(&b0);
	int val1 = gpio_pin_get_dt(&b1);

	gpio_pin_interrupt_configure_dt(&b0, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&b1, GPIO_INT_EDGE_TO_ACTIVE);


	gpio_init_callback(&b0_callback, button0_pressed, BIT(b0.pin));
	gpio_add_callback(b0.port, &b0_callback);
		
	gpio_init_callback(&b1_callback, button1_pressed, BIT(b1.pin));
	gpio_add_callback(b1.port, &b1_callback);

	printk("\nBefore sem init gpio");
	k_sem_give(&init_gpio_sem);
	k_sem_give(&init_gpio_sem);
	k_sem_give(&init_gpio_sem);
	//k_sem_give(&init_gpio_sem);
	printk("\nBefore sem alarm\n");
	//k_sem_give(&alarm_sem);
	printk("Je lance le main\n");

	init_wifi();

	wifi_connect();

	//k_sem_take(&wifi_connected, K_FOREVER);
	wifi_status();
	//k_sem_take(&ipv4_address_obtained, K_FOREVER);

	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock, ret;
	struct http_request req = { 0 };
	static uint8_t recv_buf[512];

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
	printf("getaddrinfo status: %d\n", st);
	if (st != 0) {
		printf("Unable to resolve address, quitting\n");
		return 0;
	}
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	zsock_connect(sock, res->ai_addr, res->ai_addrlen);

	req.method = HTTP_GET;
	req.url = "/home";
	req.host = HTTP_HOST;
	req.protocol = "HTTP/1.1";
	req.response = response_cb;
	req.recv_buf = recv_buf;
	req.recv_buf_len = sizeof(recv_buf);
	ret = http_client_req(sock, &req, 5000, NULL);

	zsock_close(sock);

	while (1) {
		if (flag == 1){

			k_mutex_lock(&lcd_mutex, K_FOREVER);
        	write_lcd(&dev_lcd_screen , START_ALERT_MONITORING_MSG_1, LCD_LINE_2);
        	write_lcd(&dev_lcd_screen , HELLO_MSG, LCD_LINE_1);

			printk("flag %d\n",flag);
			
			flag = 0;
			state_alarm = 1;
			k_mutex_unlock(&lcd_mutex);

			
		}

		if (flag_d == 1){

			k_mutex_lock(&lcd_mutex, K_FOREVER);
        	write_lcd(&dev_lcd_screen , STOP_ALERT_MONITORING_MSG_2, LCD_LINE_2);
        	write_lcd(&dev_lcd_screen , HELLO_MSG, LCD_LINE_1);
        	printk("flag_d %d\n",flag_d);

			flag_d = 0;
			state_alarm = 0;

			k_mutex_unlock(&lcd_mutex);

    	}
		k_sleep(K_MSEC(20));
		
	}

	return 0;
}

K_THREAD_DEFINE(sensor_channel_thread_id, 521, sensor_channel_thread, NULL, NULL, NULL, 9, 0, 0);
K_THREAD_DEFINE(alarm_thread_id, 521, alarm_thread, NULL, NULL, NULL, 9, 0, 0);
//K_THREAD_DEFINE(alarm_sound_id, 521, alarm_sound, NULL, NULL, NULL, 9, 0, 0);

//K_THREAD_DEFINE(steam_thread_id, 521, steam_thread, NULL, NULL, NULL, 9, 0, 0);