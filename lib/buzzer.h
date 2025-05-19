#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>

//Protótipos de Função
void buzzer_init();
void buzzer_desliga();
void tocar_alarme();
void desligar_alarme();
void atualizar_origem_alerta(bool agua, bool chuva);
void alarme_loop();

#endif