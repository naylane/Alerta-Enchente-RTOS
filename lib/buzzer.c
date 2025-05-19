#include "buzzer.h"

typedef enum { ALERTA_NENHUM, ALERTA_AGUA, ALERTA_CHUVA, ALERTA_AMBOS } alerta_t;

static struct {
    int pin;
    uint slice;
    uint channel;
    bool ativo;
    bool estado;
    uint32_t ultima_execucao;
    alerta_t tipo_alerta;
} buzzer = {0};

void buzzer_init(int pin) {
    buzzer.pin = pin;
    gpio_set_function(pin, GPIO_FUNC_PWM);
    buzzer.slice = pwm_gpio_to_slice_num(pin);
    buzzer.channel = pwm_gpio_to_channel(pin);
    pwm_set_clkdiv(buzzer.slice, 125.0);
    pwm_set_wrap(buzzer.slice, 1000);
    pwm_set_chan_level(buzzer.slice, buzzer.channel, 500);
    pwm_set_enabled(buzzer.slice, false);
    buzzer.ativo = false;
    buzzer.estado = false;
    buzzer.ultima_execucao = 0;
    buzzer.tipo_alerta = ALERTA_NENHUM;
}

// Função única para controlar o buzzer (liga/desliga e tipo de alerta)
void buzzer_control(bool ligar, bool agua, bool chuva) {
    buzzer.ativo = ligar;
    if (agua && chuva)      buzzer.tipo_alerta = ALERTA_AMBOS;
    else if (agua)          buzzer.tipo_alerta = ALERTA_AGUA;
    else if (chuva)         buzzer.tipo_alerta = ALERTA_CHUVA;
    else                    buzzer.tipo_alerta = ALERTA_NENHUM;

    if (!ligar) {
        pwm_set_enabled(buzzer.slice, false);
        gpio_put(buzzer.pin, 0);
        buzzer.estado = false;
    }
}

// Loop do alarme, alternando o estado do buzzer
void buzzer_loop() {
    if (!buzzer.ativo) {
        if (buzzer.estado) {
            pwm_set_enabled(buzzer.slice, false);
            gpio_put(buzzer.pin, 0);
            buzzer.estado = false;
        }
        return;
    }

    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if (agora - buzzer.ultima_execucao < 350) return;
    buzzer.ultima_execucao = agora;
    buzzer.estado = !buzzer.estado;

    if (buzzer.estado) {
        int freq = 0;
        switch (buzzer.tipo_alerta) {
            case ALERTA_AGUA:   freq = 50; break;
            case ALERTA_CHUVA:  freq = 100; break;
            case ALERTA_AMBOS:  freq = 200; break;
            default:            freq = 0;   break;
        }
        if (freq > 0) {
            uint32_t wrap = 1000000 / freq;
            pwm_set_wrap(buzzer.slice, wrap);
            pwm_set_chan_level(buzzer.slice, buzzer.channel, wrap / 2);
            pwm_set_enabled(buzzer.slice, true);
        }
    } else {
        pwm_set_enabled(buzzer.slice, false);
        gpio_put(buzzer.pin, 0);
        buzzer.estado = false;
    }
}
