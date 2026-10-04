// Testes de firmware/libraries/Lembregotchi/src/logica/humor.cpp
#include "doctest.h"
#include "logica/humor.h"

TEST_CASE("humorDaSemana: taxa de conclusao") {
  CHECK(humorDaSemana(3, 1, 0) == doctest::Approx(75));
  CHECK(humorDaSemana(0, 4, 0) == doctest::Approx(0));
  CHECK(humorDaSemana(5, 0, 0) == doctest::Approx(100));
}

TEST_CASE("humorDaSemana: sem eventos fica neutro") {
  CHECK(humorDaSemana(0, 0, 0) == doctest::Approx(HUMOR_SEM_EVENTOS));
}

TEST_CASE("humorDaSemana: bonus soma e o resultado fica entre 0 e 100") {
  CHECK(humorDaSemana(1, 1, 25) == doctest::Approx(75));   // 50 + 25
  CHECK(humorDaSemana(5, 0, 40) == doctest::Approx(100));  // não passa de 100
  CHECK(humorDaSemana(0, 5, -10) == doctest::Approx(0));   // não fica negativo
}

TEST_CASE("humorDaSemana: contagens negativas (resposta estranha da ponte) viram 0") {
  CHECK(humorDaSemana(-3, 1, 0) == doctest::Approx(0));
  CHECK(humorDaSemana(-1, -1, 0) == doctest::Approx(HUMOR_SEM_EVENTOS));
}

TEST_CASE("limitarStat: 0 a 100") {
  CHECK(limitarStat(-5) == 0);
  CHECK(limitarStat(50.5f) == doctest::Approx(50.5f));
  CHECK(limitarStat(130) == 100);
}
