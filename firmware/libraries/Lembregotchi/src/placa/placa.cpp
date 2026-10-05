#include "placa.h"

void placaIniciar() {
  pinMode(BAT_POWER_HOLD, OUTPUT);
  digitalWrite(BAT_POWER_HOLD, HIGH);
}

Arduino_GFX *placaNovaTela() {
  Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, GFX_NOT_DEFINED /* MISO */);
  return new Arduino_ST7789(bus, LCD_RST, 0 /* rotação */, true /* IPS */, TELA_W, TELA_H);
}

void placaLuz(bool ligada) { placaBrilho(ligada ? 255 : 0); }

void placaBrilho(uint8_t nivel) {
  analogWrite(LCD_BL, nivel);   // PWM: o LED pisca rápido demais para o olho ver, parecendo mais fraco
}

uint32_t placaBateriaAdcMv() { return analogReadMilliVolts(BAT_ADC); }

bool placaCarregando() {
  pinMode(BAT_CARREGANDO, INPUT_PULLUP);
  return digitalRead(BAT_CARREGANDO) == LOW;
}
