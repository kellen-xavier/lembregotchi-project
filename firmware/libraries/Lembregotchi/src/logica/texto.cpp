#include "texto.h"

#include <cstdint>
#include <cstring>

void asciiSimples(const char *in, char *out, size_t n) {
  if (n == 0) return;                                          // nem o terminador cabe
  static const char MIN[] = "aaaaaa?ceeeeiiii?nooooo??uuuu";   // 0xC3 0xA0..0xBC
  static const char MAI[] = "AAAAAA?CEEEEIIII?NOOOOO??UUUU";   // 0xC3 0x80..0x9C
  size_t j = 0;
  for (size_t i = 0; in[i] && j + 1 < n; ) {
    uint8_t c = (uint8_t)in[i];
    if (c < 0x80) { out[j++] = (char)c; i++; continue; }
    if (c == 0xC3 && ((uint8_t)in[i + 1] & 0xC0) == 0x80) {
      uint8_t d = (uint8_t)in[i + 1];
      char t = '?';
      if (d >= 0xA0 && d <= 0xBC) t = MIN[d - 0xA0];
      else if (d >= 0x80 && d <= 0x9C) t = MAI[d - 0x80];
      if (t != '?') out[j++] = t;
      i += 2;
      continue;
    }
    // outros caracteres (emojis etc.): pula o byte inicial e os de continuação (10xxxxxx),
    // sem nunca passar do fim do texto
    i++;
    while (((uint8_t)in[i] & 0xC0) == 0x80) i++;
  }
  out[j] = 0;
  // tira espaços do começo (sobram quando o título começava com emoji)
  size_t k = 0;
  while (out[k] == ' ') k++;
  if (k) memmove(out, out + k, strlen(out + k) + 1);
}
