cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS 
$ENV{ZEPHYR_BASE})
project(zephyr_smart_home)

target_sources(app PRIVATE 
    src/lcd_screen_i2c.c
    src/main.c
    src/adc_handler.c
)
