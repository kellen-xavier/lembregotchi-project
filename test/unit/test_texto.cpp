// Testes de firmware/libraries/Lembregotchi/src/logica/texto.cpp
#include "doctest.h"
#include "logica/texto.h"

#include <cstring>
#include <string>

// Atalho: converte e devolve como std::string (buffer grande o bastante)
static std::string converter(const char *entrada) {
  char saida[64];
  asciiSimples(entrada, saida, sizeof(saida));
  return saida;
}

TEST_CASE("asciiSimples: texto ASCII passa sem mudar") {
  CHECK(converter("Reuniao 14h") == "Reuniao 14h");
  CHECK(converter("") == "");
}

TEST_CASE("asciiSimples: acentos do portugues viram letra simples") {
  CHECK(converter("Reunião de orçamento") == "Reuniao de orcamento");
  CHECK(converter("ÁRVORE ÉPICA ÇÃO") == "ARVORE EPICA CAO");
  CHECK(converter("à á â ã é ê í ó ô õ ú ü") == "a a a a e e i o o o u u");
  CHECK(converter("Ñandú über café") == "Nandu uber cafe");
}

TEST_CASE("asciiSimples: emojis e marcas de check sao removidos") {
  CHECK(converter("✅ Almoço com João") == "Almoco com Joao");   // ✅ = 3 bytes
  CHECK(converter("❌ Academia") == "Academia");
  CHECK(converter("🎉🎉 Festa") == "Festa");                     // 🎉 = 4 bytes
  CHECK(converter("🎉") == "");
}

TEST_CASE("asciiSimples: letras latinas sem equivalente sao descartadas") {
  CHECK(converter("a æ b ø c") == "a  b  c");   // descarta as letras, mantém os espaços do meio
  CHECK(converter("æ ø ÷") == "");               // sobram só espaços no começo, que são removidos
}

TEST_CASE("asciiSimples: texto cortado no meio de um emoji nao passa do fim") {
  // 🎉 = F0 9F 8E 89; aqui só chegaram os 2 primeiros bytes
  CHECK(converter("Fim \xF0\x9F") == "Fim ");
  // C3 sem o segundo byte (acento cortado)
  CHECK(converter("Caf\xC3") == "Caf");
}

TEST_CASE("asciiSimples: respeita o tamanho do buffer") {
  SUBCASE("corta e sempre termina com \\0") {
    char saida[5];
    std::memset(saida, 'X', sizeof(saida));
    asciiSimples("Reuniao", saida, sizeof(saida));
    CHECK(std::string(saida) == "Reun");
  }
  SUBCASE("buffer de 1 byte: só o terminador") {
    char saida[1] = { 'X' };
    asciiSimples("abc", saida, 1);
    CHECK(saida[0] == '\0');
  }
  SUBCASE("buffer de 0 bytes: não escreve nada") {
    char saida[1] = { 'X' };
    asciiSimples("abc", saida, 0);
    CHECK(saida[0] == 'X');
  }
}
