#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"

#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/buzzer.h"
#include "lib/ws2812.h"
#include "ws2812.pio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define LED_GREEN 11
#define LED_RED  13
#define BUZZER_PIN 21
#define BUTTON_B 6

#endif