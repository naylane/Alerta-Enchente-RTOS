#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>

//Protótipos de Função
void buzzer_init(int pin);
void buzzer_control(bool ligar, bool agua, bool chuva);
void buzzer_loop();

#endif