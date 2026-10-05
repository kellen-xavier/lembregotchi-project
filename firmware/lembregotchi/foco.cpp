// Pomodoro clássico (estilo Pala Note) — ver docs/screen-and-flows/flow-pomodoro-classic.jpg
//
//   [Foco Baixo | Foco Guerreiro] ──► lista de tempos ──► "5min ⋮" + [Focar] ──► FOCO 30:58 🔊
//                                                                                │      ▲
//                                                 fim do foco: gato feliz +N ────┘      │
//                                                          ▼                            │
//                                            "Descansa Guerreiro" 04:58 + imagem ───────┘ (ciclo)
//   FOCO/DESCANSO ── clique longo ──► 😿 "Tem certeza?" ─► [Voltar ao foco] / [Sim! Desistir] → menu
//
// Não existe pausa. As regras do timer ficam em logica/pomodoro (testadas no PC).
#include <Arduino.h>

#include "config.h"
#include "app.h"
#include "foco.h"
#include "pet.h"
#include "tela.h"
#include "src/cat_sprites/cat_sprites.h"
#include "src/imagens/foco.h"   // troque a imagem com: make imagem IMG=sua-imagem.jpg

enum TelaFoco { TF_MODO, TF_LISTA, TF_PRONTO, TF_FOCO, TF_RECOMPENSA, TF_DESCANSO, TF_CONFIRMA };

#define FOCO_RECOMPENSA_MS 3000   // quanto tempo o gato feliz fica na tela

static TelaFoco     tf = TF_MODO;
static ModoPomodoro focoModo = MODO_BAIXO;
static int          focoModoSel = 0;       // 0 Foco Baixo, 1 Foco Guerreiro
static int          focoOpcao = 0;         // índice na lista de tempos do modo
static bool         focoComSom = true;     // ícone 🔊/🔇 (o som em si fica para uma etapa futura)
static int          focoConfirmaSel = 0;   // 0 Voltar ao foco (padrão: clique sem querer não desiste), 1 Desistir
static int          focoUltimosPontos = 0;
static uint32_t     focoRecompensaAte = 0;
static uint32_t     focoUltimoSeg = UINT32_MAX;
static Pomodoro     focoPomo = {};

static const char *NOME_MODO[2] = { "Foco Baixo", "Foco Guerreiro" };

static void dica(const char *texto) { textoCentro(228, texto, 1, COR_TINTA_P); }

// Caixa "25min ⋮" (o valor escolhido, com os três pontinhos de "mais opções")
static void caixaTempo(int y) {
  gfx->drawRoundRect(16, y, TELA_W - 32, 32, 10, COR_TINTA_P);
  gfx->drawRoundRect(17, y + 1, TELA_W - 34, 30, 10, COR_TINTA_P);
  char t[10];
  snprintf(t, sizeof(t), "%umin", pomodoroOpcaoMin(focoModo, focoOpcao));
  gfx->setTextSize(2);
  gfx->setTextColor(COR_TINTA_P);
  gfx->setCursor(28, y + 9);  gfx->print(t);
  gfx->setCursor(29, y + 9);  gfx->print(t);   // negrito
  for (int i = 0; i < 3; i++) gfx->fillCircle(TELA_W - 32, y + 9 + i * 7, 2, COR_TINTA_P);
}

// ─── Telas ───────────────────────────────────────────────────────────────────
static void desenharModo() {
  cabecalho("Pomodoro");
  for (int i = 0; i < 2; i++) pilula(16, 56 + i * 44, TELA_W - 32, 36, NOME_MODO[i], i == focoModoSel);
  textoCentro(156, focoModoSel == 0 ? "5 a 30 min + 5 de descanso" : "25 a 55 min + 10 de descanso",
              1, COR_TINTA_P);
  dica("PLUS proximo  BOOT escolhe  PWR menu");
}

// Lista de tempos aberta embaixo da caixa (como um menu suspenso)
static void desenharLista() {
  cabecalho("Pomodoro");
  textoCentro(48, focoModo == MODO_BAIXO ? "Selecione o foco baixo" : "Selecione o foco guerreiro",
              1, COR_TINTA_P);
  caixaTempo(60);

  const int VISIVEIS = 6, ALTURA = 21, Y0 = 98;
  int n = pomodoroNumOpcoes(focoModo);
  int topo = focoOpcao - VISIVEIS + 1;
  if (topo < 0) topo = 0;
  int linhas = n < VISIVEIS ? n : VISIVEIS;
  gfx->fillRoundRect(16, Y0 - 4, TELA_W - 32, linhas * ALTURA + 8, 10, COR_PAPEL);
  gfx->drawRoundRect(16, Y0 - 4, TELA_W - 32, linhas * ALTURA + 8, 10, COR_TINTA_P);
  for (int r = 0; r < linhas; r++) {
    int idx = topo + r, y = Y0 + r * ALTURA;
    bool sel = idx == focoOpcao;
    if (sel) gfx->fillRect(17, y, TELA_W - 34, ALTURA, COR_TINTA_P);
    char t[10];
    snprintf(t, sizeof(t), "%umin", pomodoroOpcaoMin(focoModo, idx));
    gfx->setTextSize(2);
    gfx->setTextColor(sel ? COR_PAPEL : COR_TINTA_P);
    gfx->setCursor(30, y + 3);
    gfx->print(t);
  }
}

static void desenharPronto() {
  cabecalho("Pomodoro");
  caixaTempo(56);
  textoCentro(110, focoModo == MODO_BAIXO ? "+ 5 min de descanso" : "+ 10 min de descanso", 1, COR_TINTA_P);
  pilula(60, 176, 120, 32, "Focar", true);
  dica("PLUS muda o tempo  BOOT foca  PWR volta");
}

// Alto-falante desenhado com formas: corpo + cone; com som, duas "ondas"; mudo, um X
static void desenharSom(int x, int y) {
  gfx->drawRoundRect(x, y, 44, 44, 8, COR_TINTA_P);
  gfx->fillRect(x + 9, y + 17, 6, 10, COR_TINTA_P);
  gfx->fillTriangle(x + 14, y + 22, x + 22, y + 11, x + 22, y + 33, COR_TINTA_P);
  if (focoComSom) {
    gfx->fillArc(x + 23, y + 22, 7, 5, -45, 45, COR_TINTA_P);     // 0° = direita
    gfx->fillArc(x + 23, y + 22, 13, 11, -45, 45, COR_TINTA_P);
  } else {
    gfx->drawLine(x + 27, y + 17, x + 36, y + 27, COR_TINTA_P);
    gfx->drawLine(x + 27, y + 27, x + 36, y + 17, COR_TINTA_P);
  }
}

static void desenharFoco(uint32_t agora) {
  cabecalho("Pomodoro");
  char tempo[8];
  pomodoroFormatar(pomodoroRestanteMs(focoPomo, agora), tempo, sizeof(tempo));
  textoCentro(62, tempo, 7, COR_TINTA_P);

  // Barra de progresso do foco + ciclo atual
  int larg = 180, cheio = larg * pomodoroProgresso(focoPomo, agora) / 1000;
  gfx->drawRect(30, 128, larg, 6, COR_TINTA_P);
  gfx->fillRect(30, 128, cheio, 6, COR_TINTA_P);
  char ciclo[16];
  snprintf(ciclo, sizeof(ciclo), "Ciclo %u", focoPomo.ciclo);
  textoCentro(140, ciclo, 1, COR_TINTA_P);

  desenharSom(98, 156);
  dica("BOOT som    segure para sair");
}

static void desenharRecompensa() {
  gfx->fillScreen(COR_PAPEL);
  gfx->drawBitmap((TELA_W - CAT_W) / 2, 20, cat_happy, CAT_W, CAT_H, COR_TINTA_P);
  char texto[12];
  snprintf(texto, sizeof(texto), "+%d!", focoUltimosPontos);
  textoCentro(184, texto, 3, COR_TINTA_P);
  textoCentro(214, "humor e energia", 1, COR_TINTA_P);
}

static void desenharDescanso(uint32_t agora) {
  cabecalho("Pomodoro");
  textoCentro(48, "Descansa Guerreiro", 2, COR_TINTA_P);
  char tempo[8];
  pomodoroFormatar(pomodoroRestanteMs(focoPomo, agora), tempo, sizeof(tempo));
  textoCentro(68, tempo, 2, COR_TINTA_P);
  textoCentro(69, tempo, 2, COR_TINTA_P);   // negrito
  gfx->draw16bitRGBBitmap(10, 88, (uint16_t *)img_foco, IMG_FOCO_W, IMG_FOCO_H);
}

static void desenharConfirma() {
  gfx->fillScreen(COR_PAPEL);
  for (int i = 0; i < 3; i++) gfx->drawRect(42 + i, 4 + i, 156 - 2 * i, 156 - 2 * i, COR_TINTA_P);
  gfx->drawBitmap(45, 7, cat_sad, CAT_W, CAT_H, COR_TINTA_P);
  textoCentro(168, "Tem certeza?", 2, COR_TINTA_P);
  pilula(6, 196, 112, 30, "Voltar ao foco", focoConfirmaSel == 0, 1);
  pilula(122, 196, 112, 30, "Sim! Desistir", focoConfirmaSel == 1, 1);
}

static void desenharAtual() {
  uint32_t agora = millis();
  switch (tf) {
    case TF_MODO:       desenharModo(); break;
    case TF_LISTA:      desenharLista(); break;
    case TF_PRONTO:     desenharPronto(); break;
    case TF_FOCO:       desenharFoco(agora); break;
    case TF_RECOMPENSA: desenharRecompensa(); break;
    case TF_DESCANSO:   desenharDescanso(agora); break;
    case TF_CONFIRMA:   desenharConfirma(); break;
  }
  mostrar();
}

static void irPara(TelaFoco t) {
  tf = t;
  focoUltimoSeg = UINT32_MAX;
  desenharAtual();
}

// A tela do timer certa para a fase atual
static TelaFoco telaDaFase() { return focoPomo.fase == POMO_DESCANSO ? TF_DESCANSO : TF_FOCO; }

static void iniciar() {
  pomodoroIniciar(focoPomo, pomodoroOpcaoMin(focoModo, focoOpcao), pomodoroDescansoMin(focoModo), millis());
  Serial.printf("Foco: %u min + %u de descanso\n", focoPomo.focoMin, focoPomo.descansoMin);
  irPara(TF_FOCO);
}

static void desistir() {
  Serial.printf("Foco: desistiu no ciclo %u\n", focoPomo.ciclo);
  pomodoroParar(focoPomo);
  tf = TF_MODO;
  appVoltarAoMenu();
}

// ─── Funções públicas ────────────────────────────────────────────────────────
bool focoRodando() { return focoPomo.fase != POMO_PARADO; }

void focoAbrir() {
  if (focoRodando()) { irPara(telaDaFase()); return; }
  focoModoSel = 0;
  irPara(TF_MODO);
}

void focoRedesenhar() { focoUltimoSeg = UINT32_MAX; desenharAtual(); }

void focoLoop(bool podeDesenhar) {
  uint32_t agora = millis();

  switch (pomodoroAtualizar(focoPomo, agora)) {
    case POMO_FOCO_TERMINOU:
      focoUltimosPontos = pomodoroRecompensa(focoPomo.focoMin);
      petRecompensar(focoUltimosPontos);
      if (tf == TF_FOCO) {                         // no "Tem certeza?", recompensa sem trocar a tela
        tf = TF_RECOMPENSA;
        focoRecompensaAte = agora + FOCO_RECOMPENSA_MS;
        if (podeDesenhar) desenharAtual();
      }
      return;
    case POMO_DESCANSO_TERMINOU:
      if (tf == TF_DESCANSO && podeDesenhar) irPara(TF_FOCO);
      return;
    default: break;
  }

  if (tf == TF_RECOMPENSA && (int32_t)(agora - focoRecompensaAte) >= 0) {
    if (podeDesenhar) irPara(telaDaFase()); else tf = telaDaFase();
    return;
  }

  if (!podeDesenhar || (tf != TF_FOCO && tf != TF_DESCANSO)) return;
  uint32_t seg = pomodoroRestanteMs(focoPomo, agora) / 1000;   // redesenha 1× por segundo
  if (seg == focoUltimoSeg) return;
  focoUltimoSeg = seg;
  desenharAtual();
}

void focoPlus() {
  switch (tf) {
    case TF_MODO:     focoModoSel = 1 - focoModoSel; break;
    case TF_LISTA:    focoOpcao = (focoOpcao + 1) % pomodoroNumOpcoes(focoModo); break;
    case TF_PRONTO:   tf = TF_LISTA; break;           // "⋮": reabre a lista
    case TF_CONFIRMA: focoConfirmaSel = 1 - focoConfirmaSel; break;
    default: return;                                  // no timer: nada (sair = segurar)
  }
  desenharAtual();
}

void focoBoot() {
  switch (tf) {
    case TF_MODO:
      focoModo = focoModoSel == 0 ? MODO_BAIXO : MODO_GUERREIRO;
      focoOpcao = pomodoroOpcaoPadrao(focoModo);
      tf = TF_LISTA;
      break;
    case TF_LISTA:  tf = TF_PRONTO; break;
    case TF_PRONTO: iniciar(); return;
    case TF_FOCO:   focoComSom = !focoComSom; break;  // só o ícone por enquanto
    case TF_CONFIRMA:
      if (focoConfirmaSel == 1) { desistir(); return; }
      tf = telaDaFase();
      break;
    default: return;
  }
  desenharAtual();
}

void focoPwr() {
  switch (tf) {
    case TF_MODO:     appVoltarAoMenu(); return;
    case TF_LISTA:    tf = TF_MODO; break;
    case TF_PRONTO:   tf = TF_MODO; break;
    case TF_CONFIRMA: tf = telaDaFase(); break;       // PWR = voltar ao foco
    default: return;
  }
  desenharAtual();
}

// Clique longo no timer (foco ou descanso): pergunta antes de desistir
void focoLongo() {
  if (tf != TF_FOCO && tf != TF_DESCANSO) return;
  focoConfirmaSel = 0;
  irPara(TF_CONFIRMA);
}
