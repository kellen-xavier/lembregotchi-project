// Testes de firmware/libraries/Lembregotchi/src/logica/tags.cpp
#include "doctest.h"
#include "logica/tags.h"

#include <cstring>
#include <string>

static std::string semTags(const char *titulo) {
  char out[64];
  removerTags(titulo, out, sizeof(out));
  return out;
}

TEST_CASE("removerTags: tira hashtags em qualquer posicao") {
  CHECK(semTags("Estudar React #estudo") == "Estudar React");
  CHECK(semTags("#estudo Estudar React") == "Estudar React");
  CHECK(semTags("Estudar #estudo React") == "Estudar React");
  CHECK(semTags("Treino #saude #manha") == "Treino");
}

TEST_CASE("removerTags: titulo sem tag fica igual (espacos normalizados)") {
  CHECK(semTags("Reuniao de projeto") == "Reuniao de projeto");
  CHECK(semTags("  Reuniao   de projeto  ") == "Reuniao de projeto");
  CHECK(semTags("") == "");
}

TEST_CASE("removerTags: '#' sozinho ou no meio da palavra nao e tag") {
  CHECK(semTags("Sala # 3") == "Sala # 3");
  CHECK(semTags("C# basico") == "C# basico");
  CHECK(semTags("#estudo") == "");
}

TEST_CASE("removerTags: mantem acentos e emojis (quem limpa e o asciiSimples)") {
  CHECK(semTags("✅ Revisão #estudo") == "✅ Revisão");
}

TEST_CASE("removerTags: respeita o tamanho do buffer") {
  char out[6];
  removerTags("Estudar #x React", out, sizeof(out));
  CHECK(std::string(out) == "Estud");
  char um[1] = { 'X' };
  removerTags("abc", um, 1);
  CHECK(um[0] == '\0');
  char zero[1] = { 'X' };
  removerTags("abc", zero, 0);
  CHECK(zero[0] == 'X');
}
