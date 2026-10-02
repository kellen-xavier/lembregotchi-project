#pragma once
#include <time.h>

void   redeBegin();        // conecta ao Wi-Fi (espera até WIFI_TIMEOUT_MS) e liga o relógio pela internet
bool   redeConfigurada();  // segredos.h foi preenchido?
bool   redeConectada();
time_t agoraEpoch();       // segundos desde 1970 (UTC), ou 0 se a hora ainda não é conhecida
