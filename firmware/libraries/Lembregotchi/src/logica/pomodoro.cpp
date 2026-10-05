#include "pomodoro.h"

#include <cstdio>

static const uint16_t OPCOES_BAIXO[]     = { 5, 10, 15, 20, 25, 30 };
static const uint16_t OPCOES_GUERREIRO[] = { 25, 30, 35, 40, 45, 50, 55 };

int pomodoroNumOpcoes(ModoPomodoro modo) {
  return modo == MODO_BAIXO ? (int)(sizeof(OPCOES_BAIXO) / sizeof(OPCOES_BAIXO[0]))
                            : (int)(sizeof(OPCOES_GUERREIRO) / sizeof(OPCOES_GUERREIRO[0]));
}

uint16_t pomodoroOpcaoMin(ModoPomodoro modo, int indice) {
  int n = pomodoroNumOpcoes(modo);
  if (indice < 0) indice = 0;
  if (indice >= n) indice = n - 1;
  return modo == MODO_BAIXO ? OPCOES_BAIXO[indice] : OPCOES_GUERREIRO[indice];
}

int pomodoroOpcaoPadrao(ModoPomodoro) { return 0; }   // 5 min (Baixo) / 25 min (Guerreiro)

uint16_t pomodoroDescansoMin(ModoPomodoro modo) { return modo == MODO_BAIXO ? 5 : 10; }

static uint32_t duracaoMs(const Pomodoro &p) {
  uint16_t min = p.fase == POMO_FOCO ? p.focoMin : p.fase == POMO_DESCANSO ? p.descansoMin : 0;
  return (uint32_t)min * 60000UL;
}

// Subtração sem sinal: continua certa quando o millis() dá a volta (~49 dias)
static uint32_t decorridoMs(const Pomodoro &p, uint32_t agoraMs) { return agoraMs - p.inicioMs; }

void pomodoroIniciar(Pomodoro &p, uint16_t focoMin, uint16_t descansoMin, uint32_t agoraMs) {
  p.fase = POMO_FOCO;
  p.focoMin = focoMin;
  p.descansoMin = descansoMin;
  p.ciclo = 1;
  p.inicioMs = agoraMs;
}

void pomodoroParar(Pomodoro &p) { p.fase = POMO_PARADO; }

uint32_t pomodoroRestanteMs(const Pomodoro &p, uint32_t agoraMs) {
  if (p.fase == POMO_PARADO) return 0;
  uint32_t total = duracaoMs(p), passou = decorridoMs(p, agoraMs);
  return passou >= total ? 0 : total - passou;
}

uint16_t pomodoroProgresso(const Pomodoro &p, uint32_t agoraMs) {
  uint32_t total = duracaoMs(p);
  if (total == 0) return 0;
  uint32_t passou = decorridoMs(p, agoraMs);
  if (passou >= total) return 1000;
  return (uint16_t)((uint64_t)passou * 1000 / total);
}

EventoPomodoro pomodoroAtualizar(Pomodoro &p, uint32_t agoraMs) {
  if (p.fase == POMO_PARADO) return POMO_NADA;
  if (pomodoroRestanteMs(p, agoraMs) > 0) return POMO_NADA;

  p.inicioMs = agoraMs;
  if (p.fase == POMO_FOCO) {
    if (p.descansoMin == 0) {        // sem descanso: emenda o próximo foco
      p.ciclo++;
    } else {
      p.fase = POMO_DESCANSO;
    }
    return POMO_FOCO_TERMINOU;
  }
  p.fase = POMO_FOCO;               // fim do descanso: começa o próximo ciclo
  p.ciclo++;
  return POMO_DESCANSO_TERMINOU;
}

int pomodoroRecompensa(uint16_t focoMin) {
  return focoMin > POMO_RECOMPENSA_MAX ? POMO_RECOMPENSA_MAX : focoMin;
}

void pomodoroFormatar(uint32_t restanteMs, char *buf, size_t n) {
  uint32_t seg = (restanteMs + 999) / 1000;
  if (seg > 99 * 60 + 59) seg = 99 * 60 + 59;
  snprintf(buf, n, "%02u:%02u", (unsigned)(seg / 60), (unsigned)(seg % 60));
}
