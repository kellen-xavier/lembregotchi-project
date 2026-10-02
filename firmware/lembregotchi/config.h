#pragma once

// ─── Pinos da placa (Waveshare ESP32-S3 1.54" LCD) — ver docs/arquitetura.md ──
#define LCD_DC   45
#define LCD_CS   21
#define LCD_SCK  38
#define LCD_MOSI 39
#define LCD_RST  40
#define LCD_BL   46

#define BTN_BOOT 0
#define BTN_PLUS 4
#define BTN_PWR  5

#define BAT_POWER_HOLD 2   // HIGH = mantém a placa ligada na bateria

#define TELA_W 240
#define TELA_H 240

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
