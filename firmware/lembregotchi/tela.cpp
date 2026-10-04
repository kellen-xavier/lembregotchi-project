#include "tela.h"

#include "config.h"

void textoCentro(int y, const char *txt, int tamanho, uint16_t cor) {
  gfx->setTextSize(tamanho);
  gfx->setTextColor(cor);
  gfx->setCursor((TELA_W - (int)strlen(txt) * 6 * tamanho) / 2, y);
  gfx->print(txt);
}

void mostrar() {
  gfx->flush();
}
