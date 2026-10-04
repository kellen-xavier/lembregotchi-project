#pragma once
// Tags = hashtags no título do evento ("Estudar React #estudo"). Lógica pura, sem Arduino.
// Testada em test/unit/test_tags.cpp.
#include <cstddef>

// Copia o título sem as hashtags (e sem espaços sobrando). "Estudar #estudo agora" → "Estudar agora"
void removerTags(const char *titulo, char *out, size_t n);
