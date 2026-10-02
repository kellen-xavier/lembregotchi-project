// Wi-Fi + hora pela internet (NTP). A placa não tem relógio próprio (RTC),
// então a hora certa vem de servidores públicos de horário.
#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "rede.h"
#include "segredos.h"

bool redeConfigurada() { return strlen(WIFI_SSID) > 0; }

void redeBegin() {
  if (!redeConfigurada()) {
    Serial.println("Wi-Fi: segredos.h vazio, seguindo sem rede");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  // Pede a hora a servidores NTP; a sincronização acontece sozinha quando a rede conectar
  configTzTime(FUSO_HORARIO, "pool.ntp.org", "time.google.com");

  uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < WIFI_TIMEOUT_MS) delay(100);
  if (!redeConectada()) {
    Serial.println("Wi-Fi: nao conectou (tenta de novo sozinho em segundo plano)");
    return;
  }
  Serial.printf("Wi-Fi: conectado, IP %s\n", WiFi.localIP().toString().c_str());

  // Dá alguns segundos para a primeira resposta do NTP
  inicio = millis();
  while (agoraEpoch() == 0 && millis() - inicio < 5000) delay(100);
  Serial.println(agoraEpoch() ? "Hora: sincronizada" : "Hora: ainda nao sincronizou");
}

bool redeConectada() { return WiFi.status() == WL_CONNECTED; }

time_t agoraEpoch() {
  time_t t = time(nullptr);
  return t > 1700000000 ? t : 0;   // antes de sincronizar, o relógio começa em 1970
}
