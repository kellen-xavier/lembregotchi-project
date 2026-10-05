#include "tela.h"

#include "config.h"

void textoCentro(int y, const char *txt, int tamanho, uint16_t cor) {
  gfx->setTextSize(tamanho);
  gfx->setTextColor(cor);
  gfx->setCursor((TELA_W - (int)strlen(txt) * 6 * tamanho) / 2, y);
  gfx->print(txt);
}

// "Negrito" com a fonte padrão: escreve duas vezes, deslocando 1 pixel
static void negrito(int x, int y, const char *txt, int tamanho, uint16_t cor) {
  gfx->setTextSize(tamanho);
  gfx->setTextColor(cor);
  gfx->setCursor(x, y);     gfx->print(txt);
  gfx->setCursor(x + 1, y); gfx->print(txt);
}

void cabecalho(const char *titulo) {
  gfx->fillScreen(COR_PAPEL);
  negrito(16, 12, titulo, 3, COR_TINTA_P);
  gfx->drawFastHLine(16, 42, TELA_W - 32, COR_TINTA_P);
}

void pilula(int x, int y, int w, int h, const char *texto, bool selecionada, int tamanho) {
  int r = h / 3;
  if (selecionada) {
    gfx->fillRoundRect(x, y, w, h, r, COR_TINTA_P);
  } else {
    gfx->fillRoundRect(x, y, w, h, r, COR_PAPEL);
    gfx->drawRoundRect(x, y, w, h, r, COR_TINTA_P);
    gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, r, COR_TINTA_P);   // contorno de 2 px
  }
  gfx->setTextSize(tamanho);
  gfx->setTextColor(selecionada ? COR_PAPEL : COR_TINTA_P);
  int larg = (int)strlen(texto) * 6 * tamanho, alt = 8 * tamanho;
  gfx->setCursor(x + (w - larg) / 2, y + (h - alt) / 2);
  gfx->print(texto);
}

void mostrar() {
  gfx->flush();
}
