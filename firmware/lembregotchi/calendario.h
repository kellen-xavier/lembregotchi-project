#pragma once
#include <time.h>

// Conversa com a ponte do Apps Script (apps-script/Codigo.gs).

#define AGENDA_MAX_PENDENTES 5

struct EventoPendente {
  char   id[160];
  char   titulo[44];
  time_t fim;
};

struct ResumoAgenda {
  time_t agora;        // hora do servidor
  int    criados;      // eventos novos desde a última sincronização → Comida
  int    sim;          // ✅ nos últimos 7 dias → Humor
  int    nao;          // ❌ ou esquecidos nos últimos 7 dias → Humor
  int    hoje;         // eventos de hoje que ainda não terminaram (Home); -1 = ponte antiga
  int    nPendentes;   // eventos terminados aguardando "Concluiu?" → passo 7
  EventoPendente pendentes[AGENDA_MAX_PENDENTES];
};

bool calendarioConfigurado();                               // URL e chave preenchidas em segredos.h?
// desde:  última sincronização (só conta eventos criados depois disso)
// inicio: quando o Lembregotchi começou a acompanhar a agenda (eventos anteriores são ignorados)
bool calendarioResumo(time_t desde, time_t inicio, ResumoAgenda &r);
bool calendarioCheck(const char *id, time_t fim, bool feito);
