// Testes de firmware/libraries/Lembregotchi/src/logica/bloqueio.cpp
#include "doctest.h"
#include "logica/bloqueio.h"

#include <cstdint>

TEST_CASE("deveBloquear: so depois do limite sem tocar em botao") {
  CHECK_FALSE(deveBloquear(59999, 0, 60000));
  CHECK(deveBloquear(60000, 0, 60000));
  CHECK(deveBloquear(90000, 10000, 60000));
  CHECK_FALSE(deveBloquear(69999, 10000, 60000));
}

TEST_CASE("deveBloquear: resiste a volta do millis()") {
  uint32_t ultima = UINT32_MAX - 1000;
  CHECK_FALSE(deveBloquear(ultima + 30000, ultima, 60000));
  CHECK(deveBloquear(ultima + 60000, ultima, 60000));
}
