#pragma once
// Lógica pura de texto: sem Arduino, roda no PC e é testada em test/unit/test_texto.cpp.
#include <cstddef>

// A fonte da tela só tem ASCII: troca letras acentuadas (UTF-8) pela letra sem acento
// e descarta o resto (emojis, ✅/❌ etc.). Tira espaços do começo.
// "out" recebe no máximo n−1 letras + o terminador; com n == 0 não escreve nada.
void asciiSimples(const char *in, char *out, size_t n);
