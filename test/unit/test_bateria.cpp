// Testes de firmware/libraries/Lembregotchi/src/logica/bateria.cpp
#include "doctest.h"
#include "logica/bateria.h"

TEST_CASE("bateriaMvDoAdc: divisor 1/3 da placa") {
  CHECK(bateriaMvDoAdc(1400) == 4200);
  CHECK(bateriaMvDoAdc(0) == 0);
}

TEST_CASE("bateriaNivel: tabela do firmware oficial da Waveshare") {
  CHECK(bateriaNivel(3400) == 1);
  CHECK(bateriaNivel(3519) == 1);
  CHECK(bateriaNivel(3520) == 20);    // limites: a partir de 3,52 V já é 20 %
  CHECK(bateriaNivel(3700) == 40);
  CHECK(bateriaNivel(3800) == 60);
  CHECK(bateriaNivel(3950) == 80);
  CHECK(bateriaNivel(4000) == 100);
  CHECK(bateriaNivel(4200) == 100);
}
