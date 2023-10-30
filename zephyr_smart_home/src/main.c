#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#define LED_YELLOW_NODE DT_ALIAS(led_yellow)

const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});

int main(void) {
    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);
}
