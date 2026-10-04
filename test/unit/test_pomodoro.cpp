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

TEST_CASE("pomodoro: foco simples (Selecionar Tempo) termina e para") {
  Pomodoro p;
  pomodoroIniciar(p, 5, 0, 1000);
  CHECK(p.fase == POMO_FOCO);
  CHECK(pomodoroRestanteMs(p, 1000) == 5 * MIN);
  CHECK(pomodoroAtualizar(p, 1000 + 5 * MIN - 1) == POMO_NADA);
  CHECK(pomodoroRestanteMs(p, 1000 + 5 * MIN - 1) == 1);
  CHECK(pomodoroAtualizar(p, 1000 + 5 * MIN) == POMO_FOCO_TERMINOU);
  CHECK(p.fase == POMO_PARADO);
  CHECK(pomodoroAtualizar(p, 1000 + 6 * MIN) == POMO_NADA);   // não repete o evento
}

TEST_CASE("pomodoro: classico vai de foco para pausa e depois para") {
  Pomodoro p;
  pomodoroIniciar(p, POMO_BAIXO_FOCO, POMO_BAIXO_PAUSA, 0);
  CHECK(pomodoroAtualizar(p, 25 * MIN) == POMO_FOCO_TERMINOU);
  CHECK(p.fase == POMO_PAUSA);
  CHECK(pomodoroRestanteMs(p, 25 * MIN) == 5 * MIN);          // a pausa começa cheia
  CHECK(pomodoroAtualizar(p, 29 * MIN) == POMO_NADA);
  CHECK(pomodoroAtualizar(p, 30 * MIN) == POMO_PAUSA_TERMINOU);
  CHECK(p.fase == POMO_PARADO);
}

TEST_CASE("pomodoro: pausar congela o tempo e continuar desconta o tempo parado") {
  Pomodoro p;
  pomodoroIniciar(p, 10, 0, 0);
  pomodoroAlternarPausa(p, 2 * MIN);                  // pausou com 8 min restando
  CHECK(pomodoroPausado(p));
  CHECK(pomodoroRestanteMs(p, 2 * MIN) == 8 * MIN);
  CHECK(pomodoroRestanteMs(p, 30 * MIN) == 8 * MIN);  // parado: não anda
  CHECK(pomodoroAtualizar(p, 30 * MIN) == POMO_NADA); // pausado nunca termina
  pomodoroAlternarPausa(p, 30 * MIN);                 // continua
  CHECK_FALSE(pomodoroPausado(p));
  CHECK(pomodoroRestanteMs(p, 30 * MIN) == 8 * MIN);
  CHECK(pomodoroAtualizar(p, 38 * MIN) == POMO_FOCO_TERMINOU);
}

TEST_CASE("pomodoro: pausar exatamente em millis() == 0 ainda conta como pausado") {
  Pomodoro p;
  pomodoroIniciar(p, 1, 0, 0);
  pomodoroAlternarPausa(p, 0);
  CHECK(pomodoroPausado(p));
}

TEST_CASE("pomodoro: continua certo quando o millis() da a volta (~49 dias)") {
  Pomodoro p;
  uint32_t quaseNoFim = UINT32_MAX - 30000;            // 30 s antes da volta
  pomodoroIniciar(p, 1, 0, quaseNoFim);
  uint32_t depois = quaseNoFim + 45000;                // passou da volta: vira um número pequeno
  CHECK(depois < quaseNoFim);
  CHECK(pomodoroRestanteMs(p, depois) == 15000);
  CHECK(pomodoroAtualizar(p, quaseNoFim + MIN) == POMO_FOCO_TERMINOU);
}

TEST_CASE("pomodoro: parar zera e nao gera evento") {
  Pomodoro p;
  pomodoroIniciar(p, 25, 5, 0);
  pomodoroParar(p);
  CHECK(p.fase == POMO_PARADO);
  CHECK(pomodoroRestanteMs(p, 0) == 0);
  CHECK(pomodoroAtualizar(p, 99 * MIN) == POMO_NADA);
  pomodoroAlternarPausa(p, 0);                         // pausar parado não faz nada
  CHECK_FALSE(pomodoroPausado(p));
}

TEST_CASE("pomodoro: recompensa e 1 ponto por minuto, no maximo 50") {
  CHECK(pomodoroRecompensa(5) == 5);
  CHECK(pomodoroRecompensa(25) == 25);
  CHECK(pomodoroRecompensa(50) == 50);
  CHECK(pomodoroRecompensa(60) == POMO_RECOMPENSA_MAX);
  CHECK(pomodoroRecompensa(0) == 0);
}

TEST_CASE("pomodoro: formata MM:SS arredondando para cima") {
  CHECK(formatar(25 * MIN) == "25:00");
  CHECK(formatar(4 * MIN + 49000) == "04:49");
  CHECK(formatar(4 * MIN + 48001) == "04:49");   // 4:48,001 ainda mostra 4:49
  CHECK(formatar(1) == "00:01");
  CHECK(formatar(0) == "00:00");
  CHECK(formatar(200u * MIN) == "99:59");        // nunca passa de 2 dígitos
}
