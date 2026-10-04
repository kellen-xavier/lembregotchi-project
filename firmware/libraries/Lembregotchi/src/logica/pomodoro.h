#pragma once
// Lógica pura do Pomodoro: sem Arduino. O tempo entra por parâmetro (agoraMs = millis()),
// então roda no PC e é testada em test/unit/test_pomodoro.cpp.
#include <cstddef>
#include <cstdint>

enum FasePomodoro { POMO_PARADO, POMO_FOCO, POMO_PAUSA };
enum EventoPomodoro { POMO_NADA, POMO_FOCO_TERMINOU, POMO_PAUSA_TERMINOU };

struct Pomodoro {
  FasePomodoro fase;
  uint16_t focoMin;      // duração do foco
  uint16_t pausaMin;     // duração da pausa (0 = sem pausa)
  uint32_t inicioMs;     // quando a fase atual começou
  uint32_t parouEmMs;    // quando o usuário pausou (0 = rodando)
};

// Modos prontos ("Pomodoro clássico")
#define POMO_BAIXO_FOCO       25
#define POMO_BAIXO_PAUSA       5
#define POMO_GUERREIRO_FOCO   50
#define POMO_GUERREIRO_PAUSA  10

// "Selecionar Tempo": 5, 10, … 60 min
#define POMO_OPCOES     12
#define POMO_PASSO_MIN   5

#define POMO_RECOMPENSA_MAX 50   // recompensa: 1 ponto por minuto focado, no máximo 50

void pomodoroIniciar(Pomodoro &p, uint16_t focoMin, uint16_t pausaMin, uint32_t agoraMs);
void pomodoroParar(Pomodoro &p);

// Pausa/continua a contagem (o botão "pausa" do usuário, não a fase de pausa)
void pomodoroAlternarPausa(Pomodoro &p, uint32_t agoraMs);
bool pomodoroPausado(const Pomodoro &p);

// Quanto falta na fase atual (0 quando parado ou já terminou)
uint32_t pomodoroRestanteMs(const Pomodoro &p, uint32_t agoraMs);

// Avança as fases: FOCO → PAUSA (se houver) → PARADO. Chame a cada volta do loop().
EventoPomodoro pomodoroAtualizar(Pomodoro &p, uint32_t agoraMs);

// Pontos de humor/energia por completar um foco de focoMin minutos
int pomodoroRecompensa(uint16_t focoMin);

// "MM:SS" (arredonda para cima: 299,2 s → "05:00"); buf precisa de 6 bytes ou mais
void pomodoroFormatar(uint32_t restanteMs, char *buf, size_t n);
