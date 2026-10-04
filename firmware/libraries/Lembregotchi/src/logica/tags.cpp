#include "tags.h"

#include <cstdint>

// Uma hashtag vai do '#' até o próximo espaço
static bool ehEspaco(char c) { return c == ' ' || c == '\t' || c == '\n'; }

void removerTags(const char *titulo, char *out, size_t n) {
  if (n == 0) return;
  size_t j = 0;
  bool espacoPendente = false;
  for (size_t i = 0; titulo[i]; ) {
    if (ehEspaco(titulo[i])) { espacoPendente = j > 0; i++; continue; }
    // palavra: começa com '#' (seguido de algo) → pula inteira
    bool tag = titulo[i] == '#' && titulo[i + 1] && !ehEspaco(titulo[i + 1]);
    size_t fim = i;
    while (titulo[fim] && !ehEspaco(titulo[fim])) fim++;
    if (!tag) {
      if (espacoPendente && j + 1 < n) out[j++] = ' ';
      for (size_t k = i; k < fim && j + 1 < n; k++) out[j++] = titulo[k];
      espacoPendente = false;
    }
    i = fim;
  }
  out[j] = 0;
}
