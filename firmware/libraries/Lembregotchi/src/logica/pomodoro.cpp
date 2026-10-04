#include "pomodoro.h"

#include <cstdio>

static uint32_t duracaoMs(const Pomodoro &p) {
  uint16_t min = p.fase == POMO_FOCO ? p.focoMin : p.fase == POMO_PAUSA ? p.pausaMin : 0;
  return (uint32_t)min * 60000UL;
}

// Tempo decorrido na fase. Subtração sem sinal: continua certa quando o millis() dá a volta (~49 dias).
static uint32_t decorridoMs(const Pomodoro &p, uint32_t agoraMs) {
  uint32_t ate = p.parouEmMs ? p.parouEmMs : agoraMs;
  return ate - p.inicioMs;
}

void pomodoroIniciar(Pomodoro &p, uint16_t focoMin, uint16_t pausaMin, uint32_t agoraMs) {
  p.fase = POMO_FOCO;
  p.focoMin = focoMin;
  p.pausaMin = pausaMin;
  p.inicioMs = agoraMs;
  p.parouEmMs = 0;
}

void pomodoroParar(Pomodoro &p) {
  p.fase = POMO_PARADO;
  p.parouEmMs = 0;
}

bool pomodoroPausado(const Pomodoro &p) { return p.fase != POMO_PARADO && p.parouEmMs != 0; }

void pomodoroAlternarPausa(Pomodoro &p, uint32_t agoraMs) {
  if (p.fase == POMO_PARADO) return;
  if (p.parouEmMs == 0) {
    p.parouEmMs = agoraMs ? agoraMs : 1;    // 0 significa "rodando"
  } else {
    p.inicioMs += agoraMs - p.parouEmMs;    // o tempo parado não conta
    p.parouEmMs = 0;
  }
}

uint32_t pomodoroRestanteMs(const Pomodoro &p, uint32_t agoraMs) {
  if (p.fase == POMO_PARADO) return 0;
  uint32_t total = duracaoMs(p), passou = decorridoMs(p, agoraMs);
  return passou >= total ? 0 : total - passou;
}

EventoPomodoro pomodoroAtualizar(Pomodoro &p, uint32_t agoraMs) {
  if (p.fase == POMO_PARADO || p.parouEmMs) return POMO_NADA;
  if (pomodoroRestanteMs(p, agoraMs) > 0) return POMO_NADA;

  if (p.fase == POMO_FOCO) {
    if (p.pausaMin > 0) {
      p.fase = POMO_PAUSA;
      p.inicioMs = agoraMs;
    } else {
      p.fase = POMO_PARADO;
    }
    return POMO_FOCO_TERMINOU;
  }
  p.fase = POMO_PARADO;   // fim da pausa
  return POMO_PAUSA_TERMINOU;
}

int pomodoroRecompensa(uint16_t focoMin) {
  return focoMin > POMO_RECOMPENSA_MAX ? POMO_RECOMPENSA_MAX : focoMin;
}

void pomodoroFormatar(uint32_t restanteMs, char *buf, size_t n) {
  uint32_t seg = (restanteMs + 999) / 1000;
  if (seg > 99 * 60 + 59) seg = 99 * 60 + 59;
  snprintf(buf, n, "%02u:%02u", (unsigned)(seg / 60), (unsigned)(seg % 60));
}
