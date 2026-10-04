// Teste de placa — Passo 1: acender a tela da Waveshare ESP32-S3 1.54" LCD (ST7789 240x240)
// Usa a mesma biblioteca do firmware: pinos e criação da tela vêm de <Lembregotchi.h>.
#include <Lembregotchi.h>

Arduino_GFX *gfx = placaNovaTela();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Passo 1: iniciando a tela...");

  // 1) Iniciar o chip da tela (ST7789)
  if (!gfx->begin()) {
    Serial.println("ERRO: gfx->begin() falhou!");
  }

  // 2) Acender a luz de fundo
  placaLuz(true);

  // 3) Teste de cores: vermelho, verde, azul
  gfx->fillScreen(RGB565_RED);   delay(700);
  gfx->fillScreen(RGB565_GREEN); delay(700);
  gfx->fillScreen(RGB565_BLUE);  delay(700);

  // 4) Tela final
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(120, 140, 50, RGB565_ORANGE);          // "corpo" do bichinho
  gfx->fillCircle(102, 128, 8, RGB565_BLACK);            // olho esquerdo
  gfx->fillCircle(138, 128, 8, RGB565_BLACK);            // olho direito
  gfx->fillTriangle(80, 100, 95, 60, 110, 95, RGB565_ORANGE);   // orelha esquerda
  gfx->fillTriangle(130, 95, 145, 60, 160, 100, RGB565_ORANGE); // orelha direita

  gfx->setTextColor(RGB565_YELLOW);
  gfx->setTextSize(2);
  gfx->setCursor((240 - 18 * 12) / 2, 15);  // 18 letras x 12 px, centralizado
  gfx->println("Ola, Lembregotchi! Beijo Mozão");

  Serial.println("Pronto! A tela deve mostrar o bichinho.");
}

void loop() {
  // por enquanto, nada
}
