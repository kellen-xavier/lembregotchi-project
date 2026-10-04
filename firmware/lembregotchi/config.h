#pragma once

// Pinos e tamanho da tela: biblioteca do projeto (firmware/libraries/Lembregotchi/src/placa/placa.h)
#include <Lembregotchi.h>

// ─── Bichinho (valores do nekogotchi) ────────────────────────────────────────
#define PET_START_HUNGER       85
#define PET_START_HAPPY        85
#define PET_START_ENERGY       90
#define PET_DECAY_HUNGER_PH    3    // comida cai 3 por hora
#define PET_DECAY_HAPPY_PH     2    // humor cai 2 por hora
#define PET_RECOVER_ENERGY_PH  8    // energia sobe 8 por hora
#define PET_FEED_HUNGER        50
#define PET_PLAY_HAPPY         20
#define PET_PLAY_ENERGY        15
#define PET_PET_HAPPY          10
#define PET_ACTION_MS          1400 // quanto tempo a pose da ação fica na tela
#define PET_TH_HUNGRY          30
#define PET_TH_SLEEP_ENERGY    20
#define PET_TH_SAD             15
#define PET_TH_HAPPY           70

// Acelera o tempo do bichinho para testes: 1 = tempo real, 60 = 1 minuto vale 1 hora
#define PET_VELOCIDADE         1

#define PET_SALVAR_A_CADA_MS   (5UL * 60UL * 1000UL)  // salva o estado a cada 5 min

// ─── Rede e hora ─────────────────────────────────────────────────────────────
#define FUSO_HORARIO           "<-03>3"   // horário de Brasília (UTC−3, sem horário de verão)
#define WIFI_TIMEOUT_MS        12000      // quanto esperar o Wi-Fi ao ligar
#define PET_ELAPSED_CAP_H      48         // tempo desligado conta no máximo 48 h

// ─── Google Calendar (ver docs/integracao-google-calendar.md) ────────────────
#define AGENDA_COMIDA_POR_EVENTO      20    // cada evento novo na agenda alimenta o gato
#define AGENDA_ENERGIA_POR_CHECK      15    // check "sim" sobe, "não" desce (passo 7)
#define AGENDA_HUMOR_SEM_EVENTOS      50    // humor neutro se não houve eventos na semana
#define AGENDA_RECOVER_ENERGY_PH      2     // com agenda, a energia vem dos checks; só descansa devagar
#define AGENDA_SYNC_A_CADA_MS         (5UL * 60UL * 1000UL)  // busca a agenda a cada 5 min
#define AGENDA_RETENTAR_MS            (60UL * 1000UL)        // se falhar, tenta de novo em 1 min
