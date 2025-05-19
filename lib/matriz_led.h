#ifndef MATRIZ_LED_H
#define MATRIZ_LED_H
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "ws2812.pio.h"

#define NUM_LEDS 25
#define IS_RGBW false
#define led_matrix_pin 7

// Declaração de variáveis externas
extern uint32_t leds[NUM_LEDS];
extern PIO pio;
extern int sm;

// Protótipos de funções
void iniciar_matriz_leds(PIO pio_inst, uint sm_num, uint pin);
uint8_t matriz_posicao_xy(uint8_t x, uint8_t y);
uint32_t create_color(uint8_t green, uint8_t red, uint8_t blue);
void clear_matrix(PIO pio_inst, uint sm_num);
void update_leds(PIO pio_inst, uint sm_num);
void exibir_nivel(uint8_t linhas, uint8_t r, uint8_t g, uint8_t b);
void exibir_padrao();

#endif