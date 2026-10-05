// Testes de firmware/libraries/Lembregotchi/src/logica/data.cpp
#include "doctest.h"
#include "logica/data.h"

#include <string>

static std::string curta(int d, int m) { char b[16]; dataCurta(d, m, b, sizeof(b)); return b; }
static std::string extenso(int s, int d, int m) { char b[32]; dataPorExtenso(s, d, m, b, sizeof(b)); return b; }
static std::string hora(int h, int m) { char b[8]; horaMinuto(h, m, b, sizeof(b)); return b; }

TEST_CASE("dataCurta: dia com 2 digitos e mes em maiusculas") {
  CHECK(curta(4, 10) == "04 OUT");
  CHECK(curta(27, 9) == "27 SET");
  CHECK(curta(1, 1) == "01 JAN");
  CHECK(curta(31, 12) == "31 DEZ");
}

TEST_CASE("dataPorExtenso: dia da semana em portugues, sem acentos") {
  CHECK(extenso(0, 4, 10) == "Domingo 04 Out");
  CHECK(extenso(2, 6, 10) == "Terca 06 Out");    // a fonte da tela não tem "ç"
  CHECK(extenso(6, 10, 10) == "Sabado 10 Out");
}

TEST_CASE("datas: valores fora da faixa sao limitados, sem estourar a tabela") {
  CHECK(curta(4, 0) == "04 JAN");
  CHECK(curta(4, 13) == "04 DEZ");
  CHECK(extenso(-1, 4, 10) == "Domingo 04 Out");
  CHECK(extenso(9, 4, 10) == "Sabado 04 Out");
}

TEST_CASE("horaMinuto: HH:MM") {
  CHECK(hora(9, 5) == "09:05");
  CHECK(hora(23, 59) == "23:59");
  CHECK(hora(0, 0) == "00:00");
}

TEST_CASE("datas: respeitam o tamanho do buffer") {
  char b[4];
  dataPorExtenso(0, 4, 10, b, sizeof(b));
  CHECK(std::string(b) == "Dom");
}
