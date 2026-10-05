// Testes de firmware/libraries/Lembregotchi/src/logica/pomodoro.cpp
#include "doctest.h"
#include "logica/pomodoro.h"

#include <cstdint>
#include <string>

static const uint32_t MIN = 60000;   // 1 minuto em ms

static std::string formatar(uint32_t ms) {
  char buf[8];
  pomodoroFormatar(ms, buf, sizeof(buf));
  return buf;
}

TEST_CASE("pomodoro: tempos do Foco Baixo e do Foco Guerreiro") {
  CHECK(pomodoroNumOpcoes(MODO_BAIXO) == 6);
  CHECK(pomodoroOpcaoMin(MODO_BAIXO, 0) == 5);
  CHECK(pomodoroOpcaoMin(MODO_BAIXO, 5) == 30);
  CHECK(pomodoroNumOpcoes(MODO_GUERREIRO) == 7);
  CHECK(pomodoroOpcaoMin(MODO_GUERREIRO, 0) == 25);
  CHECK(pomodoroOpcaoMin(MODO_GUERREIRO, 6) == 55);
  CHECK(pomodoroDescansoMin(MODO_BAIXO) == 5);
  CHECK(pomodoroDescansoMin(MODO_GUERREIRO) == 10);
  CHECK(pomodoroOpcaoMin(MODO_BAIXO, pomodoroOpcaoPadrao(MODO_BAIXO)) == 5);
  CHECK(pomodoroOpcaoMin(MODO_GUERREIRO, pomodoroOpcaoPadrao(MODO_GUERREIRO)) == 25);
}

TEST_CASE("pomodoro: indice fora da lista e limitado") {
  CHECK(pomodoroOpcaoMin(MODO_BAIXO, -3) == 5);
  CHECK(pomodoroOpcaoMin(MODO_BAIXO, 99) == 30);
  CHECK(pomodoroOpcaoMin(MODO_GUERREIRO, 99) == 55);
}

TEST_CASE("pomodoro: ciclo classico foco -> descanso -> foco, sem parar") {
  Pomodoro p;
  pomodoroIniciar(p, 25, 5, 0);
  CHECK(p.fase == POMO_FOCO);
  CHECK(p.ciclo == 1);
  CHECK(pomodoroAtualizar(p, 25 * MIN - 1) == POMO_NADA);
  CHECK(pomodoroAtualizar(p, 25 * MIN) == POMO_FOCO_TERMINOU);
  CHECK(p.fase == POMO_DESCANSO);
  CHECK(pomodoroRestanteMs(p, 25 * MIN) == 5 * MIN);      // o descanso começa cheio
  CHECK(pomodoroAtualizar(p, 30 * MIN) == POMO_DESCANSO_TERMINOU);
  CHECK(p.fase == POMO_FOCO);                              // volta ao foco…
  CHECK(p.ciclo == 2);                                     // …no ciclo seguinte
  CHECK(pomodoroRestanteMs(p, 30 * MIN) == 25 * MIN);
  CHECK(pomodoroAtualizar(p, 55 * MIN) == POMO_FOCO_TERMINOU);
  CHECK(p.ciclo == 2);
}

TEST_CASE("pomodoro: evento acontece uma vez so por fim de fase") {
  Pomodoro p;
  pomodoroIniciar(p, 5, 5, 0);
  CHECK(pomodoroAtualizar(p, 5 * MIN) == POMO_FOCO_TERMINOU);
  CHECK(pomodoroAtualizar(p, 5 * MIN) == POMO_NADA);
  CHECK(pomodoroAtualizar(p, 5 * MIN + 1000) == POMO_NADA);
}

TEST_CASE("pomodoro: sem descanso emenda o proximo foco") {
  Pomodoro p;
  pomodoroIniciar(p, 5, 0, 0);
  CHECK(pomodoroAtualizar(p, 5 * MIN) == POMO_FOCO_TERMINOU);
  CHECK(p.fase == POMO_FOCO);
  CHECK(p.ciclo == 2);
}

TEST_CASE("pomodoro: continua certo quando o millis() da a volta (~49 dias)") {
  Pomodoro p;
  uint32_t quaseNoFim = UINT32_MAX - 30000;            // 30 s antes da volta
  pomodoroIniciar(p, 1, 5, quaseNoFim);
  uint32_t depois = quaseNoFim + 45000;                // passou da volta: vira um número pequeno
  CHECK(depois < quaseNoFim);
  CHECK(pomodoroRestanteMs(p, depois) == 15000);
  CHECK(pomodoroAtualizar(p, quaseNoFim + MIN) == POMO_FOCO_TERMINOU);
}

TEST_CASE("pomodoro: progresso de 0 a 1000") {
  Pomodoro p;
  pomodoroIniciar(p, 10, 5, 0);
  CHECK(pomodoroProgresso(p, 0) == 0);
  CHECK(pomodoroProgresso(p, 5 * MIN) == 500);
  CHECK(pomodoroProgresso(p, 10 * MIN) == 1000);
  CHECK(pomodoroProgresso(p, 99 * MIN) == 1000);
  pomodoroParar(p);
  CHECK(pomodoroProgresso(p, 5 * MIN) == 0);
}

TEST_CASE("pomodoro: parar zera e nao gera evento") {
  Pomodoro p;
  pomodoroIniciar(p, 25, 5, 0);
  pomodoroParar(p);
  CHECK(p.fase == POMO_PARADO);
  CHECK(pomodoroRestanteMs(p, 0) == 0);
  CHECK(pomodoroAtualizar(p, 99 * MIN) == POMO_NADA);
}

TEST_CASE("pomodoro: recompensa e 1 ponto por minuto, no maximo 50") {
  CHECK(pomodoroRecompensa(5) == 5);
  CHECK(pomodoroRecompensa(25) == 25);
  CHECK(pomodoroRecompensa(50) == 50);
  CHECK(pomodoroRecompensa(55) == POMO_RECOMPENSA_MAX);
  CHECK(pomodoroRecompensa(0) == 0);
}

TEST_CASE("pomodoro: formata MM:SS arredondando para cima") {
  CHECK(formatar(25 * MIN) == "25:00");
  CHECK(formatar(4 * MIN + 58000) == "04:58");
  CHECK(formatar(4 * MIN + 57001) == "04:58");   // 4:57,001 ainda mostra 4:58
  CHECK(formatar(1) == "00:01");
  CHECK(formatar(0) == "00:00");
  CHECK(formatar(200u * MIN) == "99:59");        // nunca passa de 2 dígitos
}
