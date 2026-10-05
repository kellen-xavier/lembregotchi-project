// Navegação: Home → Menu → telas; tela de descanso por inatividade.
#include <Arduino.h>
#include <time.h>

#include "config.h"
#include "app.h"
#include "foco.h"
#include "pet.h"
#include "rede.h"
#include "tela.h"

enum Tela { T_HOME, T_MENU, T_PET, T_POMODORO, T_DESCANSO };

static Tela     appTela = T_HOME;
static Tela     appAntesDoDescanso = T_HOME;
static int      appMenuSel = 0;
static uint32_t appUltimaAtividade = 0;
static int      appUltimoMinuto = -1;     // Home/descanso: redesenha quando o minuto muda
static int      appUltimaHome = -1;       // Home: redesenha quando tarefas/eventos mudam

#define N_MENU 4
static const char *MENU[N_MENU] = { "Gato", "Agenda", "Status", "Pomodoro" };

// ─── Desenho ─────────────────────────────────────────────────────────────────

// Bateria: ícone com 5 tracinhos (20 % cada), como no Pala Note
static void desenharBateria(int x, int y) {
  int nivel = bateriaNivel(bateriaMvDoAdc(placaBateriaAdcMv()));
  gfx->drawRect(x, y, 26, 12, COR_TINTA_P);
  gfx->fillRect(x + 26, y + 3, 3, 6, COR_TINTA_P);
  int barras = nivel >= 100 ? 5 : nivel / 20;
  for (int i = 0; i < barras; i++) gfx->fillRect(x + 2 + i * 5, y + 2, 4, 8, COR_TINTA_P);
  if (placaCarregando()) {                     // raio = carregando
    gfx->drawLine(x - 8, y + 1, x - 11, y + 6, COR_TINTA_P);
    gfx->drawLine(x - 11, y + 6, x - 6, y + 6, COR_TINTA_P);
    gfx->drawLine(x - 6, y + 6, x - 9, y + 11, COR_TINTA_P);
  }
}

static bool horaAgora(struct tm &t) {
  time_t agora = agoraEpoch();
  if (!agora) return false;
  localtime_r(&agora, &t);
  return true;
}

static void linhaContador(int y, const char *rotulo, int valor) {
  gfx->setTextSize(2);
  gfx->setTextColor(COR_TINTA_P);
  gfx->setCursor(16, y + 4);
  gfx->print(rotulo);
  char n[8];
  if (valor < 0) snprintf(n, sizeof(n), "--"); else snprintf(n, sizeof(n), "%d", valor);
  gfx->setTextSize(3);
  gfx->setCursor(TELA_W - 16 - (int)strlen(n) * 18, y);
  gfx->print(n);
}

static void desenharHome() {
  gfx->fillScreen(COR_PAPEL);
  gfx->setTextSize(2);
  gfx->setTextColor(COR_TINTA_P);
  gfx->setCursor(16, 12);
  gfx->print("Menu");
  desenharBateria(TELA_W - 46, 14);
  gfx->drawFastHLine(16, 34, TELA_W - 32, COR_TINTA_P);

  struct tm t;
  char linha[24];
  if (horaAgora(t)) {
    char data[12], hora[8];
    dataCurta(t.tm_mday, t.tm_mon + 1, data, sizeof(data));
    horaMinuto(t.tm_hour, t.tm_min, hora, sizeof(hora));
    snprintf(linha, sizeof(linha), "%s  %s", data, hora);
  } else {
    snprintf(linha, sizeof(linha), "sem hora");
  }
  gfx->setCursor(16, 48);
  gfx->print(linha);

  gfx->setTextSize(5);
  gfx->setCursor(16, 72);
  gfx->print("HOJE");
  gfx->setCursor(17, 72);
  gfx->print("HOJE");                          // negrito

  gfx->drawFastHLine(16, 124, TELA_W - 32, COR_TINTA_P);
  linhaContador(138, "TAREFAS", petPendentes());
  linhaContador(176, "EVENTOS", petEventosHoje());
  textoCentro(226, "BOOT = menu", 1, COR_TINTA_P);
}

static void desenharMenu() {
  cabecalho("menu");
  for (int i = 0; i < N_MENU; i++) pilula(16, 54 + i * 44, TELA_W - 32, 36, MENU[i], i == appMenuSel);
}

// Tela de descanso: relógio grande e data por extenso, com a luz quase apagada
static void desenharDescanso() {
  gfx->fillScreen(COR_PAPEL);
  struct tm t;
  if (!horaAgora(t)) {
    textoCentro(110, "--:--", 6, COR_TINTA_P);
    return;
  }
  char hora[8], data[24];
  horaMinuto(t.tm_hour, t.tm_min, hora, sizeof(hora));
  dataPorExtenso(t.tm_wday, t.tm_mday, t.tm_mon + 1, data, sizeof(data));
  textoCentro(76, hora, 7, COR_TINTA_P);
  textoCentro(160, data, 2, COR_TINTA_P);
}

static void desenharAtual() {
  switch (appTela) {
    case T_HOME:     desenharHome(); mostrar(); break;
    case T_MENU:     desenharMenu(); mostrar(); break;
    case T_DESCANSO: desenharDescanso(); mostrar(); break;
    case T_PET:      petRedesenhar(); break;
    case T_POMODORO: focoRedesenhar(); break;
  }
}

static void irPara(Tela t) {
  appTela = t;
  appUltimoMinuto = appUltimaHome = -1;
  desenharAtual();
}

// ─── Tela de descanso ────────────────────────────────────────────────────────

static void entrarNoDescanso() {
  appAntesDoDescanso = appTela;
  placaBrilho(BLOQUEIO_BRILHO);
  irPara(T_DESCANSO);
}

// Todo botão passa por aqui. Se a tela estava em descanso, só acorda (o clique não vale como ação).
static bool acordou() {
  appUltimaAtividade = millis();
  if (appTela != T_DESCANSO) return false;
  placaBrilho(BRILHO_NORMAL);
  irPara(appAntesDoDescanso);
  return true;
}

// ─── Funções públicas ────────────────────────────────────────────────────────

void appBegin() {
  petBegin();
  appUltimaAtividade = millis();
  irPara(T_HOME);
}

void appLoop() {
  petLoop(appTela == T_PET);
  focoLoop(appTela == T_POMODORO);

  // Pomodoro rodando nunca entra em descanso: o timer é a tela
  if (appTela != T_DESCANSO && !focoRodando() &&
      deveBloquear(millis(), appUltimaAtividade, BLOQUEIO_APOS_MS)) {
    entrarNoDescanso();
    return;
  }

  // Home e descanso: atualiza o relógio a cada minuto e a Home quando os números mudam
  if (appTela == T_HOME || appTela == T_DESCANSO) {
    struct tm t;
    int minuto = horaAgora(t) ? t.tm_hour * 60 + t.tm_min : -2;
    int home = petPendentes() * 1000 + petEventosHoje();
    if (minuto != appUltimoMinuto || (appTela == T_HOME && home != appUltimaHome)) {
      appUltimoMinuto = minuto;
      appUltimaHome = home;
      desenharAtual();
    }
  }
}

void appPlus() {
  if (acordou()) return;
  switch (appTela) {
    case T_MENU:     appMenuSel = (appMenuSel + 1) % N_MENU; desenharAtual(); break;
    case T_PET:      petPlus(); break;
    case T_POMODORO: focoPlus(); break;
    default: break;
  }
}

void appBoot() {
  if (acordou()) return;
  switch (appTela) {
    case T_HOME: irPara(T_MENU); break;
    case T_MENU:
      switch (appMenuSel) {
        case 0: appTela = T_PET; petAbrirGato(); break;
        case 1: appTela = T_PET; petAbrirAgenda(); break;
        case 2: appTela = T_PET; petAbrirStatus(); break;
        case 3: appTela = T_POMODORO; focoAbrir(); break;
      }
      break;
    case T_PET:      petBoot(); break;
    case T_POMODORO: focoBoot(); break;
    default: break;
  }
}

void appPwr() {
  if (acordou()) return;
  switch (appTela) {
    case T_MENU:     irPara(T_HOME); break;
    case T_PET:      petPwr(); break;
    case T_POMODORO: focoPwr(); break;
    default: break;
  }
}

void appLongo() {
  if (acordou()) return;
  if (appTela == T_POMODORO) focoLongo();
}

void appVoltarAoMenu() { irPara(T_MENU); }
