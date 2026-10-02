// Ponte com o Google Calendar via Apps Script.
//
// Segurança:
//  - HTTPS com verificação do certificado do Google (certificados.h), nunca setInsecure();
//  - a chave vai no corpo do POST, nunca na URL, e nunca é impressa na serial;
//  - o redirecionamento do Apps Script só é seguido se apontar para script.googleusercontent.com.
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "calendario.h"
#include "certificados.h"
#include "segredos.h"

static const char *DESTINO_REDIRECIONAMENTO = "https://script.googleusercontent.com/";

bool calendarioConfigurado() {
  return strlen(LEMBREGOTCHI_URL) > 0 && strlen(LEMBREGOTCHI_CHAVE) >= 32;
}

// Faz o POST e segue (com GET) o redirecionamento que o Apps Script sempre devolve.
static bool postar(JsonDocument &pedido, JsonDocument &resposta) {
  if (!calendarioConfigurado() || WiFi.status() != WL_CONNECTED) return false;

  pedido["chave"] = LEMBREGOTCHI_CHAVE;
  String corpo;
  serializeJson(pedido, corpo);

  WiFiClientSecure cliente;
  cliente.setCACert(GOOGLE_ROOT_CA);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);   // seguimos à mão, conferindo o destino
  const char *cabecalhos[] = { "Location" };
  http.collectHeaders(cabecalhos, 1);

  if (!http.begin(cliente, LEMBREGOTCHI_URL)) return false;
  http.addHeader("Content-Type", "application/json");
  int codigo = http.POST(corpo);
  corpo = "";   // não deixa a chave sobrando na memória

  if (codigo == 301 || codigo == 302 || codigo == 303) {
    String destino = http.header("Location");
    http.end();
    if (!destino.startsWith(DESTINO_REDIRECIONAMENTO)) {
      Serial.println("Agenda: redirecionamento para destino inesperado, ignorado");
      return false;
    }
    if (!http.begin(cliente, destino)) return false;
    codigo = http.GET();
  }

  if (codigo != 200) {
    Serial.printf("Agenda: HTTP %d\n", codigo);
    http.end();
    return false;
  }

  DeserializationError erro = deserializeJson(resposta, http.getStream());
  http.end();
  if (erro) {
    Serial.printf("Agenda: resposta invalida (%s)\n", erro.c_str());
    return false;
  }
  if (!(resposta["ok"] | false)) {
    Serial.printf("Agenda: recusado (%s)\n", (const char *)(resposta["erro"] | "?"));
    return false;
  }
  return true;
}

bool calendarioResumo(time_t desde, ResumoAgenda &r) {
  JsonDocument pedido, resposta;
  pedido["acao"]  = "resumo";
  pedido["desde"] = (long long)desde;
  if (!postar(pedido, resposta)) return false;

  r.agora   = (time_t)(resposta["agora"] | 0LL);
  r.criados = resposta["criados"] | 0;
  r.sim     = resposta["semana"]["sim"] | 0;
  r.nao     = resposta["semana"]["nao"] | 0;

  r.nPendentes = 0;
  for (JsonObject p : resposta["pendentes"].as<JsonArray>()) {
    if (r.nPendentes >= AGENDA_MAX_PENDENTES) break;
    EventoPendente &e = r.pendentes[r.nPendentes++];
    strlcpy(e.id,     p["id"]     | "", sizeof(e.id));
    strlcpy(e.titulo, p["titulo"] | "", sizeof(e.titulo));
    e.fim = (time_t)(p["fim"] | 0LL);
  }
  return true;
}

bool calendarioCheck(const char *id, time_t fim, bool feito) {
  JsonDocument pedido, resposta;
  pedido["acao"]  = "check";
  pedido["id"]    = id;
  pedido["fim"]   = (long long)fim;
  pedido["feito"] = feito;
  return postar(pedido, resposta);
}
