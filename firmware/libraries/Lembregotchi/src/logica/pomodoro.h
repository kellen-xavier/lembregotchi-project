#pragma once
// Lógica pura do Pomodoro clássico: sem Arduino. O tempo entra por parâmetro (agoraMs = millis()),
// então roda no PC e é testada em test/unit/test_pomodoro.cpp.
//
//   FOCO ──fim──► DESCANSO ──fim──► FOCO (ciclo + 1) ──► …   até o usuário desistir.
//   Não existe pausa: para sair, o usuário desiste (clique longo + "Tem certeza?").
#include <cstddef>
#include <cstdint>

enum FasePomodoro { POMO_PARADO, POMO_FOCO, POMO_DESCANSO };
enum EventoPomodoro { POMO_NADA, POMO_FOCO_TERMINOU, POMO_DESCANSO_TERMINOU };
enum ModoPomodoro { MODO_BAIXO, MODO_GUERREIRO };

struct Pomodoro {
  FasePomodoro fase;
  uint16_t focoMin;      // duração de cada foco
  uint16_t descansoMin;  // duração de cada descanso
  uint16_t ciclo;        // 1 no primeiro foco, 2 no segundo…
  uint32_t inicioMs;     // quando a fase atual começou
};

#define POMO_RECOMPENSA_MAX 50   // recompensa: 1 ponto por minuto focado, no máximo 50

// Tempos de cada modo
//   Foco Baixo:     5, 10, 15, 20, 25, 30 min de foco · 5 min de descanso
//   Foco Guerreiro: 25, 30, 35, 40, 45, 50, 55 min de foco · 10 min de descanso
int      pomodoroNumOpcoes(ModoPomodoro modo);
uint16_t pomodoroOpcaoMin(ModoPomodoro modo, int indice);   // índice fora da lista → limitado
int      pomodoroOpcaoPadrao(ModoPomodoro modo);            // índice que já vem escolhido
uint16_t pomodoroDescansoMin(ModoPomodoro modo);

void pomodoroIniciar(Pomodoro &p, uint16_t focoMin, uint16_t descansoMin, uint32_t agoraMs);
void pomodoroParar(Pomodoro &p);

// Quanto falta na fase atual (0 quando parado ou já terminou)
uint32_t pomodoroRestanteMs(const Pomodoro &p, uint32_t agoraMs);

// Quanto da fase já passou, de 0 a 1000 (para a barra de progresso)
uint16_t pomodoroProgresso(const Pomodoro &p, uint32_t agoraMs);

// Avança as fases (FOCO ↔ DESCANSO). Chame a cada volta do loop().
EventoPomodoro pomodoroAtualizar(Pomodoro &p, uint32_t agoraMs);

// Pontos de humor/energia por completar um foco de focoMin minutos
int pomodoroRecompensa(uint16_t focoMin);

// "MM:SS" (arredonda para cima: 299,2 s → "05:00"); buf precisa de 6 bytes ou mais
void pomodoroFormatar(uint32_t restanteMs, char *buf, size_t n);
