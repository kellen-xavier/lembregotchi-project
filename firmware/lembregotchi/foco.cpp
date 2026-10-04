// Pomodoro ("Foco"): telas e botões.
//
//   Pomodoro (gato) ──BOOT──► menu ──► Selecionar Tempo ──► 5…60 min ──► timer
//                                 ├──► Foco Baixo      (25 min + 5 de pausa) ──► timer
//                                 └──► Foco Guerreiro  (50 min + 10 de pausa) ──► timer
//
// Fim do foco → gato feliz + recompensa (pet.cpp) → pausa (se houver) → gato.
// As regras do timer ficam em logica/pomodoro (testadas no PC).
#include <Arduino.h>

#include "config.h"
#include "foco.h"
#include "pet.h"
#include "tela.h"
#include "src/cat_sprites/cat_sprites.h"
#include "src/imagens/foco.h"   // troque a imagem com: make imagem IMG=sua-imagem.jpg

enum TelaFoco { TF_FECHADO, TF_INICIO, TF_MENU, TF_TEMPO, TF_TIMER };

static TelaFoco tf = TF_FECHADO;
static int      focoMenuSel  = 0;   // 0 Selecionar Tempo, 1 Foco Baixo, 2 Foco Guerreiro
static int      focoTempoSel = 4;   // índice da lista 5…60 min (4 = 25 min)
static Pomodoro focoPomo = {};
static uint32_t focoUltimoSeg = UINT32_MAX;   // último segundo desenhado no timer

static const uint16_t COR_ROSA  = COR(255, 205, 195);
static const uint16_t COR_LILAS = COR(220, 205, 255);

static const char *OPCOES[3]    = { "Selecionar Tempo", "Foco Baixo", "Foco Guerreiro" };
static const char *DESCRICAO[3] = { "voce escolhe de 5 a 60 min", "25 min de foco + 5 de pausa",
                                    "50 min de foco + 10 de pausa" };

static void dica(const char *texto) { textoCentro(230, texto, 1, COR_CINZA); }

// ─── Telas ───────────────────────────────────────────────────────────────────
static void desenharInicio() {
  gfx->fillScreen(COR_ROSA);
  textoCentro(14, "Pomodoro", 3, COR_TINTA);
  gfx->fillRoundRect(37, 50, 166, 166, 12, RGB565_WHITE);
  gfx->drawBitmap(45, 58, cat_content, CAT_W, CAT_H, COR_TINTA);
  dica("BOOT = escolher   PWR = voltar");
}

static void desenharMenu() {
  gfx->fillScreen(COR_LILAS);
  textoCentro(14, "Pomodoro", 3, COR_TINTA);
  gfx->fillRoundRect(12, 54, 216, 132, 10, RGB565_WHITE);
  for (int i = 0; i < 3; i++) {
    int y = 64 + i * 40;
    bool sel = i == focoMenuSel;
    if (sel) gfx->fillRoundRect(18, y - 6, 204, 30, 8, COR_TINTA);
    textoCentro(y + 2, OPCOES[i], 2, sel ? RGB565_WHITE : COR_TINTA);
  }
  textoCentro(200, DESCRICAO[focoMenuSel], 1, COR_TINTA);
  dica("PLUS proximo   BOOT ok   PWR voltar");
}

// Lista 5…60 min: mostra 7 linhas e rola para manter a escolhida visível
static void desenharTempo() {
  const int VISIVEIS = 7, ALTURA = 26;
  gfx->fillScreen(COR_LILAS);
  textoCentro(10, "Selecionar Tempo", 2, COR_TINTA);
  gfx->fillRoundRect(50, 34, 140, 190, 10, RGB565_WHITE);

  int topo = focoTempoSel - VISIVEIS / 2;
  if (topo > POMO_OPCOES - VISIVEIS) topo = POMO_OPCOES - VISIVEIS;
  if (topo < 0) topo = 0;
  for (int r = 0; r < VISIVEIS; r++) {
    int idx = topo + r;
    int y = 40 + r * ALTURA;
    bool sel = idx == focoTempoSel;
    if (sel) gfx->fillRoundRect(56, y, 128, ALTURA - 2, 6, COR_TINTA);
    char linha[10];
    snprintf(linha, sizeof(linha), "%2d min", (idx + 1) * POMO_PASSO_MIN);
    textoCentro(y + 5, linha, 2, sel ? RGB565_WHITE : COR_TINTA);
  }
  dica("PLUS proximo  BOOT inicia  PWR volta");
}

static void desenharTimer(uint32_t agora) {
  gfx->fillScreen(COR_LILAS);

  char tempo[8];
  pomodoroFormatar(pomodoroRestanteMs(focoPomo, agora), tempo, sizeof(tempo));
  textoCentro(12, tempo, 4, COR_TINTA);

  gfx->fillRoundRect(6, 50, 228, 158, 10, RGB565_WHITE);
  if (focoPomo.fase == POMO_FOCO) {
    gfx->draw16bitRGBBitmap(10, 54, (uint16_t *)img_foco, IMG_FOCO_W, IMG_FOCO_H);
  } else {
    gfx->drawBitmap(45, 54, cat_sleep, CAT_W, CAT_H, COR_TINTA);   // pausa: o gato descansa
  }

  bool pausado = pomodoroPausado(focoPomo);
  if (pausado) {
    gfx->fillRoundRect(50, 112, 140, 34, 8, COR_TINTA);
    textoCentro(121, "PAUSADO", 2, RGB565_WHITE);
  }

  textoCentro(214, focoPomo.fase == POMO_FOCO ? "Foco" : "Pausa", 1, COR_TINTA);
  dica(pausado ? "BOOT = continuar   PWR = sair" : "BOOT = pausar   PWR = sair");
}

static void desenharAtual() {
  switch (tf) {
    case TF_INICIO: desenharInicio(); break;
    case TF_MENU:   desenharMenu();   break;
    case TF_TEMPO:  desenharTempo();  break;
    case TF_TIMER:  desenharTimer(millis()); break;
    default: return;
  }
  mostrar();
}

static void iniciar(uint16_t focoMin, uint16_t pausaMin) {
  pomodoroIniciar(focoPomo, focoMin, pausaMin, millis());
  tf = TF_TIMER;
  focoUltimoSeg = UINT32_MAX;
  Serial.printf("Foco: %u min + %u de pausa\n", focoMin, pausaMin);
  desenharAtual();
}

static void fechar() {
  pomodoroParar(focoPomo);
  tf = TF_FECHADO;
  petVoltarDoFoco();
}

// ─── Funções públicas ────────────────────────────────────────────────────────
bool focoAtivo() { return tf != TF_FECHADO; }

void focoAbrir() {
  tf = TF_INICIO;
  focoMenuSel = 0;
  desenharAtual();
}

void focoRedesenhar() {
  focoUltimoSeg = UINT32_MAX;
  desenharAtual();
}

void focoLoop(bool podeDesenhar) {
  if (tf != TF_TIMER) return;
  uint32_t agora = millis();

  EventoPomodoro ev = pomodoroAtualizar(focoPomo, agora);
  if (ev == POMO_FOCO_TERMINOU) {
    bool vemPausa = focoPomo.fase == POMO_PAUSA;
    if (!vemPausa) tf = TF_FECHADO;
    focoUltimoSeg = UINT32_MAX;
    petFocoTerminou(pomodoroRecompensa(focoPomo.focoMin), vemPausa);
    return;
  }
  if (ev == POMO_PAUSA_TERMINOU) {
    tf = TF_FECHADO;
    petPausaTerminou();
    return;
  }

  if (!podeDesenhar) return;
  uint32_t seg = pomodoroRestanteMs(focoPomo, agora) / 1000;   // redesenha 1× por segundo
  if (seg == focoUltimoSeg) return;
  focoUltimoSeg = seg;
  desenharTimer(agora);
  mostrar();
}

void focoPlus() {
  switch (tf) {
    case TF_INICIO: tf = TF_MENU; break;
    case TF_MENU:   focoMenuSel = (focoMenuSel + 1) % 3; break;
    case TF_TEMPO:  focoTempoSel = (focoTempoSel + 1) % POMO_OPCOES; break;
    default: return;   // no timer, PLUS não faz nada
  }
  desenharAtual();
}

void focoBoot() {
  switch (tf) {
    case TF_INICIO: tf = TF_MENU; break;
    case TF_MENU:
      if (focoMenuSel == 1) { iniciar(POMO_BAIXO_FOCO, POMO_BAIXO_PAUSA); return; }
      if (focoMenuSel == 2) { iniciar(POMO_GUERREIRO_FOCO, POMO_GUERREIRO_PAUSA); return; }
      tf = TF_TEMPO;
      break;
    case TF_TEMPO:
      iniciar((focoTempoSel + 1) * POMO_PASSO_MIN, 0);
      return;
    case TF_TIMER:
      pomodoroAlternarPausa(focoPomo, millis());
      focoUltimoSeg = UINT32_MAX;
      break;
    default: return;
  }
  desenharAtual();
}

void focoPwr() {
  switch (tf) {
    case TF_INICIO: fechar(); return;
    case TF_MENU:   tf = TF_INICIO; break;
    case TF_TEMPO:  tf = TF_MENU; break;
    case TF_TIMER:  // desistir: sem recompensa
      Serial.println("Foco: desistiu");
      fechar();
      return;
    default: return;
  }
  desenharAtual();
}
