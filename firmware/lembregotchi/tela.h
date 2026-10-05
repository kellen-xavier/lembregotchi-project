#pragma once
// Utilitários de desenho compartilhados pelas telas (pet.cpp, foco.cpp).
#include <Arduino_GFX_Library.h>

extern Arduino_GFX *gfx;   // Canvas criado em lembregotchi.ino

// Converte r,g,b (0–255) para o formato de cor da tela (RGB565)
#define COR(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define COR_TINTA   COR(60, 40, 30)     // marrom do gato e dos textos
#define COR_CREME   COR(255, 244, 214)
#define COR_CINZA   COR(140, 120, 100)

// Estilo "Pala Note" (menus, Home, Pomodoro): fundo branco, texto preto
#define COR_PAPEL   RGB565_WHITE
#define COR_TINTA_P RGB565_BLACK

// Escreve um texto centralizado (a fonte padrão tem 6 px de largura por letra)
void textoCentro(int y, const char *txt, int tamanho, uint16_t cor);

// Cabeçalho Pala Note: título em negrito à esquerda + linha embaixo (ocupa y 0–44)
void cabecalho(const char *titulo);

// "Pílula" Pala Note: contorno arredondado; selecionada = preenchida de preto com texto branco
void pilula(int x, int y, int w, int h, const char *texto, bool selecionada, int tamanho = 2);

// Mostra o quadro: o canvas é desenhado na memória e enviado de uma vez (sem piscar)
void mostrar();
