// Lembregotchi — bichinho virtual na Waveshare ESP32-S3 1.54" LCD
//
// Navegação (app.cpp): Home → menu (Gato · Agenda · Status · Pomodoro); tela de descanso após 1 min.
//
// Botões, em geral:
//   PLUS  → próximo item
//   BOOT  → escolher / confirmar
//   PWR   → voltar
//   segurar qualquer um (clique longo) → no timer do Pomodoro, "Tem certeza?" (sair)
#include <Arduino_GFX_Library.h>
#include <OneButton.h>

#include "config.h"
#include "app.h"
#include "rede.h"

// Canvas: desenha numa "folha" na memória e manda para a tela de uma vez (sem piscar)
Arduino_GFX *gfx = new Arduino_Canvas(TELA_W, TELA_H, placaNovaTela());

OneButton botaoBoot(BTN_BOOT, true, true);
OneButton botaoPlus(BTN_PLUS, true, true);
OneButton botaoPwr(BTN_PWR, true, true);

void setup() {
  placaIniciar();

  Serial.begin(115200);

  if (!gfx->begin()) {
    Serial.println("ERRO: tela nao iniciou");
  }
  placaLuz(true);

  // Tela de espera enquanto conecta
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_YELLOW);
  gfx->setCursor((TELA_W - 13 * 12) / 2, 110);
  gfx->print("Conectando...");
  gfx->flush();
  redeBegin();

  botaoPlus.attachClick(appPlus);
  botaoBoot.attachClick(appBoot);
  botaoPwr.attachClick(appPwr);
  botaoPlus.attachLongPressStart(appLongo);
  botaoBoot.attachLongPressStart(appLongo);
  botaoPwr.attachLongPressStart(appLongo);

  appBegin();
  Serial.println("Lembregotchi pronto!");
}

void loop() {
  botaoBoot.tick();
  botaoPlus.tick();
  botaoPwr.tick();
  appLoop();
  delay(10);
}
