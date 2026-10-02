// Passo 2 — ler os botões da Waveshare ESP32-S3 1.54" LCD
// Pinos conferidos no exemplo oficial da Waveshare (02_button_example e 01_factory).
#include <Arduino_GFX_Library.h>
#include <OneButton.h>

// Tela (passo 1)
#define LCD_DC   45
#define LCD_CS   21
#define LCD_SCK  38
#define LCD_MOSI 39
#define LCD_RST  40
#define LCD_BL   46

// Botões: ficam em HIGH soltos e vão para LOW quando apertados ("ativo em LOW")
#define BTN_BOOT 0
#define BTN_PLUS 4
#define BTN_PWR  5

// Mantém a placa ligada quando está na bateria (HIGH = ligada, LOW = desliga)
#define BAT_POWER_HOLD 2

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, 240, 240);

// true = botão ativo em LOW, true = liga o resistor interno de pull-up
OneButton botaoBoot(BTN_BOOT, true, true);
OneButton botaoPlus(BTN_PLUS, true, true);
OneButton botaoPwr(BTN_PWR, true, true);

int totalEventos = 0;

// Desenha na tela qual botão foi apertado e de que jeito
void mostrarEvento(const char *botao, const char *evento, uint16_t cor) {
  totalEventos++;
  Serial.printf("[%d] %s -> %s\n", totalEventos, botao, evento);

  gfx->fillRect(0, 70, 240, 120, RGB565_BLACK);  // apaga só a área do meio

  gfx->setTextSize(4);
  gfx->setTextColor(cor);
  gfx->setCursor((240 - strlen(botao) * 24) / 2, 85);
  gfx->print(botao);

  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor((240 - strlen(evento) * 12) / 2, 140);
  gfx->print(evento);

  gfx->fillRect(0, 205, 240, 20, RGB565_BLACK);
  gfx->setTextColor(RGB565_DARKGREY);
  gfx->setCursor(70, 210);
  gfx->printf("eventos: %d", totalEventos);
}

void setup() {
  pinMode(BAT_POWER_HOLD, OUTPUT);
  digitalWrite(BAT_POWER_HOLD, HIGH);

  Serial.begin(115200);

  gfx->begin();
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_YELLOW);
  gfx->setCursor((240 - 15 * 12) / 2, 15);
  gfx->print("Teste de botoes");
  gfx->setTextColor(RGB565_DARKGREY);
  gfx->setCursor((240 - 20 * 12) / 2, 40);  // 20 letras x 12 px = largura toda
  gfx->print("aperte BOOT/PLUS/PWR");

  // BOOT
  botaoBoot.attachClick([]() { mostrarEvento("BOOT", "clique", RGB565_CYAN); });
  botaoBoot.attachDoubleClick([]() { mostrarEvento("BOOT", "duplo clique", RGB565_CYAN); });
  botaoBoot.attachLongPressStart([]() { mostrarEvento("BOOT", "clique longo", RGB565_CYAN); });

  // PLUS
  botaoPlus.attachClick([]() { mostrarEvento("PLUS", "clique", RGB565_GREEN); });
  botaoPlus.attachDoubleClick([]() { mostrarEvento("PLUS", "duplo clique", RGB565_GREEN); });
  botaoPlus.attachLongPressStart([]() { mostrarEvento("PLUS", "clique longo", RGB565_GREEN); });

  // PWR — por enquanto só mostra; não desliga a placa
  botaoPwr.attachClick([]() { mostrarEvento("PWR", "clique", RGB565_MAGENTA); });
  botaoPwr.attachDoubleClick([]() { mostrarEvento("PWR", "duplo clique", RGB565_MAGENTA); });
  botaoPwr.attachLongPressStart([]() { mostrarEvento("PWR", "clique longo", RGB565_MAGENTA); });
}

void loop() {
  // tick() olha o botão e decide se foi clique, duplo ou longo
  botaoBoot.tick();
  botaoPlus.tick();
  botaoPwr.tick();
  delay(10);
}
