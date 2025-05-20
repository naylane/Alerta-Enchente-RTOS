#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/buzzer.h"
#include "lib/ws2812.h"
#include "ws2812.pio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

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

// Estrutura de posição do joystick
typedef struct {
    uint16_t x_pos;
    uint16_t y_pos;
} joystick_data_t;

// Estrutura de informação da enchente
typedef struct {
    joystick_data_t data;
    bool alerta_ativo;
} status_t;

// Filas
QueueHandle_t xQueueJoystickData;
QueueHandle_t xQueueStatus;

void vModoTask(void *params);
void vJoystickTask(void *params);
void vDisplayTask(void *params);
void vMatrizLEDTask(void *params);
void vBuzzerTask(void *params);
void vLedRGBTask(void *params);
void gpio_irq_handler(uint gpio, uint32_t events);


int main() {
    // Ativa BOOTSEL via botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    // Inicializa fila
    xQueueJoystickData = xQueueCreate(5, sizeof(joystick_data_t));
    xQueueStatus = xQueueCreate(5, sizeof(status_t));

    // Cria as tarefas
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


// Define se está em modo alerta de enchente ou normal, baseado no joystick.
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


// Lê valores do joystick (ADC) e envia para a fila.
void vJoystickTask(void *params) {
    adc_gpio_init(JOYSTICK_Y);
    adc_gpio_init(JOYSTICK_X);
    adc_init();

    joystick_data_t joydata;

    while (true) {
        adc_select_input(0); // Volume de água
        joydata.y_pos = adc_read();

        adc_select_input(1); //Volume de chuva
        joydata.x_pos = adc_read();

        xQueueSend(xQueueJoystickData, &joydata, 0); // Envia o valor do joystick para a fila
        vTaskDelay(pdMS_TO_TICKS(100));              // 10 Hz de leitura
    }
}


// Mostra informações no display OLED conforme o status do sistema.
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

    char agua_str[20];
    char chuva_str[20];

    bool cor = true;
    while (true){
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY) == pdTRUE){
               uint pct_agua = (status_atual.data.y_pos * 100) / 4095;
               uint pct_chuva = (status_atual.data.x_pos * 100) / 4095;
               sprintf(agua_str, "Agua: %d %%", pct_agua);
               sprintf(chuva_str, "Chuva: %d %%", pct_chuva);

               if (status_atual.alerta_ativo){
                    ssd1306_fill(&ssd, false);
                    ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
                    ssd1306_line(&ssd, 3, 20, 122, 20, true);
                    ssd1306_line(&ssd, 3, 40, 122, 40, true);
                    
                    ssd1306_draw_string(&ssd, "  MONITORADOR", 5, 5);
                    ssd1306_draw_string(&ssd, "  Enchente!", 15, 26);
                    ssd1306_draw_string(&ssd, chuva_str, 5, 44);
                    ssd1306_draw_string(&ssd, agua_str, 5, 54);
               } else {
                    ssd1306_fill(&ssd, false);
                    ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
                    ssd1306_line(&ssd, 3, 20, 122, 20, true);
                    ssd1306_line(&ssd, 3, 40, 122, 40, true);
                    
                    ssd1306_draw_string(&ssd, "  MONITORADOR", 5, 5);
                    ssd1306_draw_string(&ssd, "Estado Normal", 15, 26);
                    ssd1306_draw_string(&ssd, chuva_str, 5, 44);
                    ssd1306_draw_string(&ssd, agua_str, 5, 54);
               }
               ssd1306_send_data(&ssd);
        }                       
        vTaskDelay(pdMS_TO_TICKS(50));         
    }
}


// Exibe animações na matriz de LEDs conforme o estado do sistema. Exclamação em vermelho para alerta e gota de água em azul para normal.
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
                set_pattern(pio, sm, 1, "vermelho");
                vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(250));
            } else {
                set_pattern(pio, sm, 0, "azul");
            }
        }
    }
}


// Controla o buzzer conforme o status de alerta.
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
                buzzer_control(true, agua, chuva);
            } else {
                buzzer_control(false, false, false);
            }
        }
        buzzer_loop();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}


// Controla os LEDs RGB para indicar alerta ou estado normal.
void vLedRGBTask(void *params) {
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, 0);

    status_t status_atual;
    while (true) {
        if (xQueueReceive(xQueueStatus, &status_atual, portMAX_DELAY) == pdTRUE){            
            if (status_atual.alerta_ativo) {
                gpio_put(LED_GREEN, 0);
                gpio_put(LED_RED, 1);
            } else {
                gpio_put(LED_GREEN, 1);
                gpio_put(LED_RED, 0);
            }
            
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}


// Reinicia o sistema via botão B (modo BOOTSEL).
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

