// Passo 1 — acender a tela da Waveshare ESP32-S3 1.54" LCD (ST7789 240x240)
// Pinos conferidos no exemplo oficial da Waveshare (04_gfx_helloworld).
#include <Arduino_GFX_Library.h

#define LCD_DC   45
#define LCD_CS   21
#define LCD_SCK  38
#define LCD_MOSI 39
#define LCD_RST  40
#define LCD_BL   46  // luz de fundo

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0 /* rotação */, true /* IPS */, 240, 240);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Passo 1: iniciando a tela...");

  // 1) Iniciar o chip da tela (ST7789)
  if (!gfx->begin()) {
    Serial.println("ERRO: gfx->begin() falhou!");
  }

  // 2) Acender a luz de fundo
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

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
