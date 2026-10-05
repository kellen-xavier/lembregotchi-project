#pragma once
// Hardware da Waveshare ESP32-S3 1.54" LCD — pinos conferidos no código oficial da Waveshare.
// Mapa completo: docs/arquitetura.md
#include <Arduino_GFX_Library.h>

// Tela ST7789 (SPI)
#define LCD_DC   45
#define LCD_CS   21
#define LCD_SCK  38
#define LCD_MOSI 39
#define LCD_RST  40
#define LCD_BL   46   // luz de fundo

#define TELA_W 240
#define TELA_H 240

// Botões: HIGH soltos, LOW apertados ("ativos em LOW")
#define BTN_BOOT 0    // segurar ao ligar = modo de gravação
#define BTN_PLUS 4
#define BTN_PWR  5

#define BAT_POWER_HOLD 2   // HIGH = mantém a placa ligada na bateria
#define BAT_ADC        1   // tensão da bateria ÷ 3 (ver logica/bateria.h)
#define BAT_CARREGANDO 3   // LOW = carregando (entrada com pull-up)

// Primeira coisa do setup(): mantém a placa ligada quando está só na bateria.
void placaIniciar();

// Cria o objeto da tela (ainda não iniciado: chame begin() nele ou no Canvas que o envolve).
Arduino_GFX *placaNovaTela();

// Liga/desliga a luz de fundo. Sem ela, a tela parece apagada mesmo desenhando.
void placaLuz(bool ligada);

// Brilho da luz de fundo, 0 (apagada) a 255 (máximo), por PWM
void placaBrilho(uint8_t nivel);

// Leitura crua do ADC da bateria, em mV (multiplique por 3: bateriaMvDoAdc)
uint32_t placaBateriaAdcMv();
bool placaCarregando();
