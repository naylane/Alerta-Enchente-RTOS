#include "matriz_led.h"

uint32_t leds[NUM_LEDS];
PIO pio = pio0;
int sm = 0;

bool led_estado = true;

//Inicia a matriz
void iniciar_matriz_leds(PIO pio_inst, uint sm_num, uint pin) {
    pio = pio_inst;
    sm = sm_num;
    
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    
    uint offset = pio_add_program(pio, &ws2812_program);
    printf("Loaded PIO program at offset %d\n", offset);
    ws2812_program_init(pio, sm, offset, pin, 800000, IS_RGBW);

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = 0; 
    }
    clear_matrix(pio, sm);
    update_leds(pio, sm);
    
    printf("Matriz de LEDs inicializada com sucesso!\n");
}

//Função pra localizar LEDs 
uint8_t matriz_posicao_xy(uint8_t x, uint8_t y) {
    if (y % 2 == 0) {
        return y * 5 + x;
    } else {
        return y * 5 + (4 - x);
    }
}

// Função para criar uma cor
uint32_t create_color(uint8_t green, uint8_t red, uint8_t blue) {
    return ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
}

// Limpa todos os LEDs
void clear_matrix(PIO pio_inst, uint sm_num) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = 0;
    }
}

//Atualiza a matriz
void update_leds(PIO pio_inst, uint sm_num) {
    for (int i = 0; i < NUM_LEDS; i++) {
        pio_sm_put_blocking(pio_inst, sm_num, leds[i] << 8u);
    }
}

//Exibe padrão específico
void exibir_padrao() {
    clear_matrix(pio, sm);

    if (led_estado) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = create_color(0, 80, 0);
        }
        } else {
            for (int i = 0; i < NUM_LEDS; i++) {
                    leds[i] = 0;
            }
        }
        led_estado = !led_estado;
        update_leds(pio, sm);
}

void exibir_nivel(uint8_t linhas, uint8_t r, uint8_t g, uint8_t b) {
    clear_matrix(pio, sm);

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int pos = matriz_posicao_xy(x, y); 
            if (y < linhas) {
                leds[pos] = create_color(g, r, b); 
            } else {
                leds[pos] = 0;
            }
        }
    }
    update_leds(pio, sm);
}
