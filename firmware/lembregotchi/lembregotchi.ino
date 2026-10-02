// Lembregotchi — bichinho virtual na Waveshare ESP32-S3 1.54" LCD
//
// Controles:
//   PLUS  → escolhe a próxima ação (Comer, Brincar, Carinho, Agenda, Status)
//   BOOT  → executa a ação escolhida
//   na tela de Status, qualquer botão volta
//   na tela "Concluiu?": BOOT = sim, PLUS = não, PWR = depois
#include <Arduino_GFX_Library.h>
#include <OneButton.h>

#include "config.h"
#include "pet.h"
#include "rede.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *tela = new Arduino_ST7789(bus, LCD_RST, 0, true, TELA_W, TELA_H);

// Canvas: desenha numa "folha" na memória e manda para a tela de uma vez (sem piscar)
Arduino_GFX *gfx = new Arduino_Canvas(TELA_W, TELA_H, tela);

OneButton botaoBoot(BTN_BOOT, true, true);
OneButton botaoPlus(BTN_PLUS, true, true);
OneButton botaoPwr(BTN_PWR, true, true);

void setup() {
  pinMode(BAT_POWER_HOLD, OUTPUT);
  digitalWrite(BAT_POWER_HOLD, HIGH);

  Serial.begin(115200);

  if (!gfx->begin()) {
    Serial.println("ERRO: tela nao iniciou");
  }
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  // Tela de espera enquanto conecta
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_YELLOW);
  gfx->setCursor((TELA_W - 13 * 12) / 2, 110);
  gfx->print("Conectando...");
  gfx->flush();
  redeBegin();

  botaoPlus.attachClick(petPlus);
  botaoBoot.attachClick(petBoot);
  botaoPwr.attachClick(petPwr);

  petBegin();
  Serial.println("Lembregotchi pronto!");
}

void loop() {
  botaoBoot.tick();
  botaoPlus.tick();
  botaoPwr.tick();
  petLoop();
  delay(10);
}
