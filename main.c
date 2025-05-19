//Bibliotecas inclusas
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/buzzer.h"
#include "lib/ws2812.h"
#include "ws2812.pio.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

//Configuração de pinos
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_JOYSTICK_X 26
#define ADC_JOYSTICK_Y 27
#define LED_GREEN 11
#define LED_RED  13
#define BUZZER_PIN 21

#define BUTTON_B 6

//Estrutura de posição do joystick
typedef struct {
    uint16_t x_pos;
    uint16_t y_pos;
} joystick_data_t;

//Estrutura de informação da enchente
typedef struct {
    joystick_data_t data;
    bool alerta_ativo;
} status_t;

//Definição das filas
QueueHandle_t xQueueJoystickData;
QueueHandle_t xQueueStatus;

//Tarefa para definir modo (enchente ou não)
void vModoTask(void *params) {

    status_t status_atual;
    joystick_data_t joydata;

    while (true){
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE){
            uint16_t agua = joydata.y_pos;
            uint16_t chuva = joydata.x_pos;
            if (agua >= 2866 || chuva >= 3276){ //Definição dos numeros de acordo com a porcentagem do enunciado
                status_atual.data = joydata;
                status_atual.alerta_ativo = true;
            } else {
                status_atual.data = joydata;
                status_atual.alerta_ativo = false;
            }
        xQueueSend(xQueueStatus, &status_atual, 0); 
        vTaskDelay(pdMS_TO_TICKS(100));   
        }
    }
}

//Tarefa de leitura do ADC do Joystick
void vJoystickTask(void *params) {
    adc_gpio_init(ADC_JOYSTICK_Y);
    adc_gpio_init(ADC_JOYSTICK_X);
    adc_init();

    joystick_data_t joydata;

    while (true)
    {
        adc_select_input(0); // Volume de água
        joydata.y_pos = adc_read();

        adc_select_input(1); //Volume de chuva
        joydata.x_pos = adc_read();

        xQueueSend(xQueueJoystickData, &joydata, 0); // Envia o valor do joystick para a fila
        vTaskDelay(pdMS_TO_TICKS(100));              // 10 Hz de leitura
    }
}

//Tarefa de exibição de infos no display
void vDisplayTask(void *params) {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    status_t status_atual;

    char pctAgua_str[32];
    char pctChuva_str[32];

    bool cor = true;
    while (true){
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY) == pdTRUE){
               uint pct_agua = (status_atual.data.y_pos * 100) / 4095;
               uint pct_chuva = (status_atual.data.x_pos * 100) / 4095;
               sprintf(pctAgua_str, "Nvl agua: %d", pct_agua);
                sprintf(pctChuva_str, "Nvl chuva: %d", pct_chuva);

               if (status_atual.alerta_ativo){
                    ssd1306_fill(&ssd, false);
                    ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
                    ssd1306_line(&ssd, 3, 20, 122, 20, true);
                    ssd1306_line(&ssd, 3, 40, 122, 40, true);
                    
                    ssd1306_draw_string(&ssd, "Modo: Enchente", 5, 5);
                    ssd1306_draw_string(&ssd, "ALERTA!", 38, 26);
                    ssd1306_draw_string(&ssd, pctAgua_str, 5, 42);
                    ssd1306_draw_string(&ssd, pctChuva_str, 5, 52);
               } else {
                    ssd1306_fill(&ssd, false);
                    ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
                    ssd1306_line(&ssd, 3, 20, 122, 20, true);
                    ssd1306_line(&ssd, 3, 40, 122, 40, true);
                    
                    ssd1306_draw_string(&ssd, "Modo: Normal", 5, 5);
                    ssd1306_draw_string(&ssd, "Tudo bem!", 32, 26);
                    ssd1306_draw_string(&ssd, pctAgua_str, 5, 42);
                    ssd1306_draw_string(&ssd, pctChuva_str, 5, 52);
               }
               ssd1306_send_data(&ssd);
        }                       
        vTaskDelay(pdMS_TO_TICKS(50));         
    }
}

//Tarefa do LED RGB com PWM
void vLedRGBTask(void *params) {
    gpio_set_function(LED_RED, GPIO_FUNC_PWM);   // Configura GPIO como PWM
    uint redSlice = pwm_gpio_to_slice_num(LED_RED); // Obtém o slice de PWM
    pwm_set_wrap(redSlice, 100);                     // Define resolução (0–100)
    pwm_set_chan_level(redSlice, PWM_CHAN_B, 0);     // Duty inicial
    pwm_set_enabled(redSlice, true);                 // Ativa PWM

    gpio_set_function(LED_GREEN, GPIO_FUNC_PWM);   
    uint greenSlice = pwm_gpio_to_slice_num(LED_GREEN); 
    pwm_set_wrap(greenSlice, 100);                     
    pwm_set_chan_level(greenSlice, PWM_CHAN_A, 0);     
    pwm_set_enabled(greenSlice, true);                 

    status_t status_atual;
    while (true)
    {
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY) == pdTRUE){
            int nivel_x = (status_atual.data.x_pos * 100) / 4095;
            int nivel_y = (status_atual.data.y_pos * 100) / 4095;
            int nivel_alerta = (nivel_x > nivel_y) ? nivel_x : nivel_y;
            
            if (nivel_alerta < 60) {
                pwm_set_chan_level(greenSlice, PWM_CHAN_B, 100);
                pwm_set_chan_level(redSlice, PWM_CHAN_B, 0);
            } else {
                int progresso = nivel_alerta - 60;
                if (progresso > 20) progresso = 20; 
                int vermelho = (progresso * 100) / 20; 
                int verde = 100 - vermelho;

                pwm_set_chan_level(redSlice, PWM_CHAN_B, vermelho);
                pwm_set_chan_level(greenSlice, PWM_CHAN_B, verde);
            }
            
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}

//Tarefa que configura o buzzer
void vBuzzerTask(void *params) {
    buzzer_init(BUZZER_PIN); 
    status_t status_atual;
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();

    while(true){
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY) == pdTRUE){
            if (status_atual.alerta_ativo){
                bool agua = status_atual.data.y_pos >= 2866;
                bool chuva = status_atual.data.x_pos >= 3276;
                atualizar_origem_alerta(agua, chuva);
                tocar_alarme();
            } else {
                desligar_alarme();
            }
        }
        alarme_loop();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

//Tarefa que exibe animações na matriz de LED
void vMatrizLEDTask(void *params) {
    // Inicializa o PIO para controlar a matriz de LEDs (WS2812)
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, sm, offset, WS2812_PIN);
    clear_matrix(pio, sm);

    status_t status_atual;
    TickType_t lastWakeTime;

    while(true){
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY) == pdTRUE){
            lastWakeTime = xTaskGetTickCount();
            if (status_atual.alerta_ativo){
                set_pattern(pio, sm, 1, "vermelho"); // ALERTA
                vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(250));
            } else {
                set_pattern(pio, sm, 0, "azul");
            }
        }
    }
}

// Modo BOOTSEL com botão B
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}


//Função principal
int main() {
    // Ativa BOOTSEL via botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    // Cria a fila para compartilhamento de valor do joystick
    xQueueJoystickData = xQueueCreate(5, sizeof(joystick_data_t));
    xQueueStatus = xQueueCreate(5, sizeof(status_t));

    // Criação das tasks
    xTaskCreate(vJoystickTask, "Joystick Task", 256, NULL, 1, NULL);
    xTaskCreate(vModoTask, "Modo Task", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedRGBTask, "LED RGB Task", 256, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", 256, NULL, 1, NULL);
    xTaskCreate(vMatrizLEDTask, "Matriz Task", 256, NULL, 1, NULL);
    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
