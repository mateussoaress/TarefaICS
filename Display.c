// Bibliotecas

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"
#include "pio_matrix.pio.h"

// Definição dos Pinos
#define BOTAO_A 5
#define BOTAO_B 6
#define LED_VERMELHO 13
#define LED_VERDE 11
#define LED_AZUL 12
#define PORTA_I2C i2c1
#define SDA_I2C 14
#define SCL_I2C 15
#define ENDERECO_I2C 0x3C
#define MATRIZ_LED 7

// Variáveis globais
volatile int numero_exibido = 0;  // Número exibido na matriz
PIO pio = pio0;
int estado_maquina = 0;
ssd1306_t ssd;

// Protótipos das funções
void debounce_botao(uint pino);
void mostrar_numero(int numero);
void atualizar_display(const char *mensagem);
void verificar_botoes();
void verificar_entrada_usb();
void configurar();

int main() {
    configurar();
    while (1) {
        verificar_botoes();
        verificar_entrada_usb();
        sleep_ms(40); // Pequeno atraso para evitar processamento excessivo
    }
}

// Matriz com os padrões dos números
const uint32_t numeros[10][25] = {
    { // 0
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 1,
      1, 0, 0, 0, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1 },
    { // 1
      0, 0, 1, 0, 0,
      0, 0, 1, 1, 0,
      0, 0, 1, 0, 0,
      0, 0, 1, 0, 0,
      0, 0, 1, 0, 0 },
    { // 2
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1,
      0, 0, 0, 0, 1,
      1, 1, 1, 1, 1 },
    { // 3
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1 },
    { // 4
      1, 0, 0, 0, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      0, 0, 0, 0, 1 },
    { // 5
      1, 1, 1, 1, 1,
      0, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1 },
    { // 6
      1, 1, 1, 1, 1,
      0, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1 },
    { // 7
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      0, 0, 0, 1, 0,
      0, 0, 1, 0, 0,
      0, 0, 1, 0, 0 },
    { // 8
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1 },
    { // 9
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1 }
};

// Evita leituras erradas do botão
void debounce_botao(uint pino) {
    sleep_ms(50);
    while (!gpio_get(pino)) {
        sleep_ms(10);
    }
}

// Implementa a exibição do número na matriz de LEDs
void mostrar_numero(int numero) {
    float brilho = 0.1; 
    uint32_t cores[10] = {
        0xFF0000, 0x00FF00, 0x00FFFF, 0xFFFF00, 
        0xFF00FF, 0x00FFFF, 0xFFA500, 0x8A2BE2, 
        0xFFFFFF, 0x808080 
    };

    uint8_t r = ((cores[numero] >> 16) & 0xFF) * brilho;
    uint8_t g = ((cores[numero] >> 8) & 0xFF) * brilho;
    uint8_t b = (cores[numero] & 0xFF) * brilho;

    uint32_t cor = (r << 16) | (g << 8) | b; 

    for (int i = 0; i < 25; i++) {
        uint32_t cor_pixel = numeros[numero][24 - i] ? cor : 0x000000;
        pio_sm_put_blocking(pio, estado_maquina, cor_pixel);
    }
}

// Atualiza o display OLED com a mensagem fornecida
void atualizar_display(const char *mensagem) {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, mensagem, 8, 10);
    ssd1306_send_data(&ssd);
    sleep_ms(10); 
}

// Implementa a lógica para interação com os botões
void verificar_botoes() {
    static bool estado_anterior_A = true;
    static bool estado_anterior_B = true;

    bool estado_atual_A = gpio_get(BOTAO_A);
    bool estado_atual_B = gpio_get(BOTAO_B);

    if (!estado_atual_A && estado_anterior_A) {
        debounce_botao(BOTAO_A);
        if (gpio_get(LED_AZUL)) gpio_put(LED_AZUL, 0);
        gpio_put(LED_VERDE, !gpio_get(LED_VERDE));
        const char *status = gpio_get(LED_VERDE) ? "Ligado" : "Desligado";
        printf("Botão A pressionado: LED Verde %s\n", status);
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "LED Verde: %s", status);
        atualizar_display(buffer);
    }
    
    if (!estado_atual_B && estado_anterior_B) {
        debounce_botao(BOTAO_B);
        if (gpio_get(LED_VERDE)) gpio_put(LED_VERDE, 0);
        gpio_put(LED_AZUL, !gpio_get(LED_AZUL));
        const char *status = gpio_get(LED_AZUL) ? "Ligado" : "Desligado";
        printf("Botão B pressionado: LED Azul %s\n", status);
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "LED Azul: %s", status);
        atualizar_display(buffer);
    }

    estado_anterior_A = estado_atual_A;
    estado_anterior_B = estado_atual_B;
}

// Implementa a captura de caracteres via USB
void verificar_entrada_usb() {
    static bool usb_conectado_anteriormente = false;

    if (stdio_usb_connected()) {
        if (!usb_conectado_anteriormente) {
            printf("Digite um caractere para exibir no display:\n");
            usb_conectado_anteriormente = true;
        }

        int resultado = getchar_timeout_us(0);
        if (resultado != PICO_ERROR_TIMEOUT) {
            char caractere = (char) resultado;
            printf("Caractere recebido: %c\n", caractere);
            char buffer[2] = {caractere, '\0'};
            atualizar_display(buffer);
            
            if (caractere >= '0' && caractere <= '9') {
                int numero = caractere - '0';
                mostrar_numero(numero);
            } else {
                printf("Apagando matriz de LEDs\n");
                for (int i = 0; i < 25; i++) {
                    pio_sm_put_blocking(pio, estado_maquina, 0x000000);
                }
            }
        }
    } else {
        usb_conectado_anteriormente = false;
    }
}

// Inicializa os periféricos do microcontrolador
void configurar() {
    stdio_init_all();
    
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);
    
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    
    i2c_init(PORTA_I2C, 400 * 1000);
    gpio_set_function(SDA_I2C, GPIO_FUNC_I2C);
    gpio_set_function(SCL_I2C, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_I2C);
    gpio_pull_up(SCL_I2C);
    ssd1306_init(&ssd, 128, 64, false, ENDERECO_I2C, PORTA_I2C);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    uint deslocamento = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, estado_maquina, deslocamento, MATRIZ_LED);
}





