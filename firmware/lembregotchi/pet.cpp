// Lembregotchi — lógica e desenho do bichinho.
// Baseado em pet.cpp do nekogotchi (https://github.com/defcon1702/pala-nekogotchi, MIT).
// Mudanças: tela colorida 240x240 em vez de e-paper, botões PLUS/BOOT, estado salvo
// na flash (Preferences) e hora real pela internet (NTP) — a placa não tem RTC.
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>

#include "config.h"
#include "pet.h"
#include "rede.h"
#include "calendario.h"
#include "app.h"
#include "tela.h"
#include "src/cat_sprites/cat_sprites.h"

// ─── Estado ──────────────────────────────────────────────────────────────────
// Guardado como float para a queda lenta (ex.: 3 por hora) acumular a cada segundo.
static float    petHunger = PET_START_HUNGER;   // comida: 100 = de barriga cheia
static float    petHappy  = PET_START_HAPPY;    // humor
static float    petEnergy = PET_START_ENERGY;   // energia
static uint32_t petIdadeSeg = 0;                // tempo de vida contado com a placa ligada
static time_t   petNasc  = 0;                   // quando nasceu (hora real), 0 = ainda não sabe
static time_t   petVisto = 0;                   // última vez que o estado foi salvo (hora real)
static bool     petHoraAplicada = false;        // já descontou o tempo em que ficou desligado?
static float    petBonusHumor = 0;              // humor extra (Pomodoro) somado à taxa da semana; cai com o tempo

// Agenda
static time_t   petSyncDesde = 0;               // hora (do servidor) da última sincronização
static time_t   petAgendaInicio = 0;            // quando começou a acompanhar a agenda
static uint32_t petProximoSync = 0;             // millis() da próxima tentativa
static time_t   petAgendaHora = 0;              // quando sincronizou com sucesso pela última vez
static ResumoAgenda petAgenda = {};             // último resumo; pendentes[0] é o próximo "Concluiu?"

enum PetView { PV_MAIN, PV_STATS, PV_ACTION, PV_CHECK };
static PetView  petView = PV_MAIN;
static int      petSel  = 0;           // índice em ACOES
static uint32_t petActionUntil = 0;
static bool     petVoltarParaCheck = false;   // depois da pose, volta para o próximo pendente
static bool     petVoltarAoMenu    = false;   // depois da pose, volta ao menu (veio de lá)
static bool     petCheckDoMenu     = false;   // "Concluiu?" aberto pelo menu (Agenda) → sai para o menu
static bool     petAtivo           = false;   // a tela do gato/agenda/status está na frente?

static uint32_t petUltimoTick   = 0;
static uint32_t petUltimoSalvar = 0;
static int      petUltimoDesenho = -1; // "assinatura" do que está na tela

// Agenda, Status e Pomodoro ficam no menu (app.cpp); no gato, só as ações de cuidar dele
#define N_ACOES 3
enum Acao { A_COMER, A_BRINCAR, A_CARINHO };
static const char *ACOES[N_ACOES] = { "Comer", "Brincar", "Carinho" };

static void petAbrirCheck();   // definida mais abaixo

static Preferences prefs;

// ─── Utilidades ──────────────────────────────────────────────────────────────
static inline float clampStat(float v) { return limitarStat(v); }   // logica/humor (testada)

// ─── Salvar e carregar (memória flash, sobrevive a desligar) ─────────────────
static void petSave() {
  prefs.putFloat("hunger", petHunger);
  prefs.putFloat("happy",  petHappy);
  prefs.putFloat("energy", petEnergy);
  prefs.putUInt("idade",   petIdadeSeg);
  time_t agora = agoraEpoch();
  if (agora) petVisto = agora;
  prefs.putLong64("nasc",  petNasc);
  prefs.putLong64("visto", petVisto);
  prefs.putLong64("sync",  petSyncDesde);
  prefs.putLong64("inicio", petAgendaInicio);
  prefs.putFloat("bonus", petBonusHumor);
  petUltimoSalvar = millis();
}

static void petLoad() {
  petHunger   = clampStat(prefs.getFloat("hunger", PET_START_HUNGER));
  petHappy    = clampStat(prefs.getFloat("happy",  PET_START_HAPPY));
  petEnergy   = clampStat(prefs.getFloat("energy", PET_START_ENERGY));
  petIdadeSeg = prefs.getUInt("idade", 0);
  petNasc     = prefs.getLong64("nasc", 0);
  petVisto    = prefs.getLong64("visto", 0);
  petSyncDesde = prefs.getLong64("sync", 0);
  petAgendaInicio = prefs.getLong64("inicio", 0);
  petBonusHumor = prefs.getFloat("bonus", 0);
}

// ─── Passagem do tempo ───────────────────────────────────────────────────────
static void aplicarHoras(float horas) {
  petHunger = clampStat(petHunger - PET_DECAY_HUNGER_PH   * horas);
  petHappy  = clampStat(petHappy  - PET_DECAY_HAPPY_PH    * horas);
  petBonusHumor -= PET_DECAY_HAPPY_PH * horas;
  if (petBonusHumor < 0) petBonusHumor = 0;
  float recupera = calendarioConfigurado() ? AGENDA_RECOVER_ENERGY_PH : PET_RECOVER_ENERGY_PH;
  petEnergy = clampStat(petEnergy + recupera * horas);
}

// Roda uma vez, quando a hora real fica conhecida: desconta o tempo em que a placa
// ficou desligada (entre o último salvamento e o momento em que ligou).
static void petAplicarTempoDesligado() {
  if (petHoraAplicada) return;
  time_t agora = agoraEpoch();
  if (agora == 0) return;
  petHoraAplicada = true;

  time_t ligouEm = agora - millis() / 1000;   // o tempo ligado já foi contado pelo millis()
  if (petNasc == 0) petNasc = ligouEm - petIdadeSeg;

  if (petVisto > 0 && ligouEm > petVisto) {
    long desligado = (long)(ligouEm - petVisto);
    long limite = (long)PET_ELAPSED_CAP_H * 3600L;
    if (desligado > limite) desligado = limite;
    aplicarHoras(desligado / 3600.0f);
    Serial.printf("Ficou desligado %ld min — tempo descontado\n", desligado / 60);
  }
  petSave();
}

static void petTick() {
  uint32_t agora = millis();
  uint32_t dt = agora - petUltimoTick;
  if (dt < 1000) return;              // atualiza uma vez por segundo
  petUltimoTick = agora;

  aplicarHoras((dt / 3600000.0f) * PET_VELOCIDADE);
  petAplicarTempoDesligado();
  petIdadeSeg += (dt / 1000) * PET_VELOCIDADE;

  if (agora - petUltimoSalvar >= PET_SALVAR_A_CADA_MS) petSave();
}

// ─── Agenda → Comida e Humor ─────────────────────────────────────────────────
// Comida: +20 por evento criado desde a última sincronização.
// Humor:  taxa de conclusão (✅ / (✅ + ❌)) dos últimos 7 dias + bônus do Pomodoro.
// Energia vem dos checks no aparelho (passo 7).
static void petSincronizarAgenda() {
  if (!calendarioConfigurado() || !redeConectada() || agoraEpoch() == 0) return;
  if ((int32_t)(millis() - petProximoSync) < 0) return;

  // Na primeira vez, "agora" vira o início: a agenda antiga não deixa o gato triste
  time_t inicio = petAgendaInicio ? petAgendaInicio : agoraEpoch();

  ResumoAgenda r;
  if (!calendarioResumo(petSyncDesde, inicio, r)) {
    petProximoSync = millis() + AGENDA_RETENTAR_MS;
    return;
  }
  petProximoSync = millis() + AGENDA_SYNC_A_CADA_MS;

  if (r.criados > 0) petHunger = clampStat(petHunger + r.criados * AGENDA_COMIDA_POR_EVENTO);

  petHappy = humorDaSemana(r.sim, r.nao, petBonusHumor);

  bool chegouPendente = r.nPendentes > petAgenda.nPendentes;
  petAgenda = r;
  petAgendaInicio = inicio;
  petAgendaHora = agoraEpoch();
  if (r.agora > 0) petSyncDesde = r.agora;   // próxima vez, só o que vier depois disto
  petSave();

  Serial.printf("Agenda: %d novos, semana %d sim / %d nao, %d pendentes\n",
                r.criados, r.sim, r.nao, r.nPendentes);

  // Evento novo terminou: o gato pergunta sozinho. Na tela de check, atualiza a lista.
  if (!petAtivo) return;
  if (petView == PV_CHECK || (chegouPendente && petView == PV_MAIN)) petAbrirCheck();
}

// ─── Humor → desenho e cores ─────────────────────────────────────────────────
enum Humor { H_CONTENTE, H_FELIZ, H_FOME, H_TRISTE, H_SONO };

static Humor petHumor() {
  if (petEnergy < PET_TH_SLEEP_ENERGY)                      return H_SONO;
  if (petHappy  < PET_TH_SAD && petHunger < PET_TH_SAD)     return H_TRISTE;
  if (petHunger < PET_TH_HUNGRY)                            return H_FOME;
  if (petHunger > PET_TH_HAPPY && petHappy > PET_TH_HAPPY)  return H_FELIZ;
  return H_CONTENTE;
}

static const uint8_t *spriteDoHumor(Humor h) {
  switch (h) {
    case H_SONO:   return cat_sleep;
    case H_TRISTE: return cat_sad;
    case H_FOME:   return cat_hungry;
    case H_FELIZ:  return cat_happy;
    default:       return cat_content;
  }
}

// Fundo da tela muda conforme o humor; o gato é desenhado na cor "tinta"
static void coresDoHumor(Humor h, uint16_t &fundo, uint16_t &tinta) {
  switch (h) {
    case H_SONO:   fundo = COR(30, 35, 70);    tinta = COR(225, 225, 255); break;
    case H_TRISTE: fundo = COR(190, 205, 230); tinta = COR(40, 50, 80);    break;
    case H_FOME:   fundo = COR(255, 214, 170); tinta = COR(80, 40, 20);    break;
    case H_FELIZ:  fundo = COR(200, 240, 190); tinta = COR(30, 70, 30);    break;
    default:       fundo = COR(255, 244, 214); tinta = COR(60, 40, 30);    break;
  }
}

static const uint16_t COR_COMIDA  = COR(240, 110, 60);
static const uint16_t COR_HUMOR   = COR(250, 200, 40);
static const uint16_t COR_ENERGIA = COR(70, 150, 240);

// ─── Desenho ─────────────────────────────────────────────────────────────────
static void barrinha(int x, int y, int w, int h, float valor, uint16_t cor, uint16_t contorno) {
  gfx->drawRoundRect(x, y, w, h, 3, contorno);
  int cheio = (int)((w - 4) * valor / 100.0f);
  if (cheio > 0) gfx->fillRoundRect(x + 2, y + 2, cheio, h - 4, 2, cor);
}

static void petDrawMain() {
  Humor h = petHumor();
  uint16_t fundo, tinta;
  coresDoHumor(h, fundo, tinta);

  gfx->fillScreen(fundo);

  // Três barrinhas no topo: comida, humor, energia
  barrinha(12,  8, 66, 12, petHunger, COR_COMIDA,  tinta);
  barrinha(87,  8, 66, 12, petHappy,  COR_HUMOR,   tinta);
  barrinha(162, 8, 66, 12, petEnergy, COR_ENERGIA, tinta);

  // O gato (150x150) no centro
  gfx->drawBitmap((TELA_W - CAT_W) / 2, 26, spriteDoHumor(h), CAT_W, CAT_H, tinta);

  // Aviso de eventos esperando "Concluiu?"
  if (petAgenda.nPendentes > 0) {
    gfx->fillCircle(206, 44, 13, COR(220, 50, 50));
    char n[4];
    snprintf(n, sizeof(n), "%d", petAgenda.nPendentes);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(201, 37);
    gfx->print(n);
  }

  // Ação escolhida, com setinhas dos lados
  gfx->fillRoundRect(30, 184, 180, 34, 10, tinta);
  gfx->fillTriangle(42, 201, 52, 193, 52, 209, fundo);     // ◀
  gfx->fillTriangle(198, 201, 188, 193, 188, 209, fundo);  // ▶
  textoCentro(194, ACOES[petSel], 2, fundo);

  // Bolinhas mostrando em qual das ações estamos
  for (int i = 0; i < N_ACOES; i++) {
    int cx = TELA_W / 2 - (N_ACOES - 1) * 7 + i * 14;
    if (i == petSel) gfx->fillCircle(cx, 229, 4, tinta);
    else             gfx->drawCircle(cx, 229, 4, tinta);
  }
}

static void petDrawStats() {
  uint16_t fundo = COR(255, 244, 214), tinta = COR(60, 40, 30);
  gfx->fillScreen(fundo);
  textoCentro(6, "Status", 3, tinta);

  struct { const char *nome; float valor; uint16_t cor; } linhas[3] = {
    { "Comida",  petHunger, COR_COMIDA  },
    { "Humor",   petHappy,  COR_HUMOR   },
    { "Energia", petEnergy, COR_ENERGIA },
  };
  for (int i = 0; i < 3; i++) {
    int y = 38 + i * 48;
    gfx->setTextSize(2);
    gfx->setTextColor(tinta);
    gfx->setCursor(16, y);
    gfx->print(linhas[i].nome);
    gfx->setCursor(TELA_W - 16 - 3 * 12, y);
    gfx->printf("%3d", (int)linhas[i].valor);
    barrinha(16, y + 20, TELA_W - 32, 16, linhas[i].valor, linhas[i].cor, tinta);
  }

  // Idade: pela hora real, se conhecida; senão, pelo tempo ligado
  time_t agora = agoraEpoch();
  uint32_t vidaSeg = (agora && petNasc && agora > petNasc) ? (uint32_t)(agora - petNasc) : petIdadeSeg;
  uint32_t horas = vidaSeg / 3600;
  char linha[24];
  snprintf(linha, sizeof(linha), "Idade: %lud %luh", (unsigned long)(horas / 24), (unsigned long)(horas % 24));
  textoCentro(176, linha, 2, tinta);

  // Rede e hora
  const uint16_t VERDE = COR(40, 130, 60), VERMELHO = COR(180, 60, 40);
  if (agora) {
    struct tm t;
    localtime_r(&agora, &t);
    snprintf(linha, sizeof(linha), "Wi-Fi ok  %02d:%02d", t.tm_hour, t.tm_min);
  } else {
    snprintf(linha, sizeof(linha), redeConfigurada() ? "Sem Wi-Fi" : "Wi-Fi nao config.");
  }
  textoCentro(198, linha, 2, agora ? VERDE : VERMELHO);

  // Agenda
  bool agendaOk = petAgendaHora && agora && agora - petAgendaHora < 30 * 60;
  if (!calendarioConfigurado())  snprintf(linha, sizeof(linha), "Agenda nao config.");
  else if (agendaOk)             snprintf(linha, sizeof(linha), "Agenda ok  %d pend.", petAgenda.nPendentes);
  else                           snprintf(linha, sizeof(linha), "Agenda sem sinal");
  textoCentro(220, linha, 2, agendaOk ? VERDE : VERMELHO);
}

static void petStartAction(const uint8_t *sprite, uint16_t fundo, const char *texto) {
  petView = PV_ACTION;
  petActionUntil = millis() + PET_ACTION_MS;

  uint16_t tinta = COR(60, 40, 30);
  gfx->fillScreen(fundo);
  gfx->drawBitmap((TELA_W - CAT_W) / 2, 30, sprite, CAT_W, CAT_H, tinta);
  textoCentro(196, texto, 3, tinta);
}

// Redesenha a tela principal só quando algo visível mudou
static void petRedesenharSePreciso(bool forcar) {
  // Junta humor, ação e os três valores (0–100) num número só para comparar
  int assinatura = ((((int)petHumor() * N_ACOES + petSel) * (AGENDA_MAX_PENDENTES + 1)
                     + petAgenda.nPendentes) * 101 + (int)petHunger) * 101 * 101
                   + (int)petHappy * 101 + (int)petEnergy;
  if (!forcar && assinatura == petUltimoDesenho) return;
  petUltimoDesenho = assinatura;
  petDrawMain();
  mostrar();
}

// ─── Check: "Concluiu?" ──────────────────────────────────────────────────────

// Quebra o título em até 3 linhas de 18 letras (tamanho 2 = 12 px por letra)
static void desenharTitulo(const char *titulo, int y, uint16_t cor) {
  const int MAX = 18;
  char resto[48];
  strlcpy(resto, titulo, sizeof(resto));
  char *p = resto;
  for (int linha = 0; linha < 3 && *p; linha++) {
    char buf[MAX + 1];
    int len = strlen(p);
    int corte = len;
    if (len > MAX) {
      corte = MAX;
      while (corte > 0 && p[corte] != ' ') corte--;   // quebra no espaço
      if (corte == 0) corte = MAX;                   // palavra enorme: corta no meio
      if (linha == 2) {                              // última linha: reticências
        corte = MAX - 3;
        memcpy(buf, p, corte);
        strcpy(buf + corte, "...");
        textoCentro(y + linha * 22, buf, 2, cor);
        return;
      }
    }
    memcpy(buf, p, corte);
    buf[corte] = 0;
    textoCentro(y + linha * 22, buf, 2, cor);
    p += corte;
    while (*p == ' ') p++;
  }
}

static void petDrawCheck() {
  uint16_t fundo = COR(255, 244, 214), tinta = COR(60, 40, 30);
  gfx->fillScreen(fundo);

  const EventoPendente &e = petAgenda.pendentes[0];
  textoCentro(8, "Concluiu?", 3, tinta);

  char linha[24];
  snprintf(linha, sizeof(linha), "1 de %d", petAgenda.nPendentes);
  textoCentro(38, linha, 1, COR(140, 120, 100));

  char titulo[48];
  asciiSimples(e.titulo, titulo, sizeof(titulo));
  if (!titulo[0]) strlcpy(titulo, "(sem titulo)", sizeof(titulo));
  gfx->drawRoundRect(10, 54, TELA_W - 20, 76, 8, tinta);
  desenharTitulo(titulo, 62, tinta);

  // Quando terminou
  struct tm t;
  time_t fim = e.fim;
  localtime_r(&fim, &t);
  snprintf(linha, sizeof(linha), "terminou %02d:%02d", t.tm_hour, t.tm_min);
  textoCentro(138, linha, 2, COR(140, 120, 100));

  // Botões
  gfx->fillRoundRect(20, 164, 200, 28, 8, COR(60, 150, 70));
  textoCentro(171, "BOOT = sim", 2, RGB565_WHITE);
  gfx->fillRoundRect(20, 198, 200, 28, 8, COR(190, 70, 60));
  textoCentro(205, "PLUS = nao", 2, RGB565_WHITE);
  textoCentro(230, "PWR = depois", 1, COR(140, 120, 100));
}

// Sai do "Concluiu?" / Status: volta para onde veio (menu ou gato)
static void petSairParaOrigem() {
  if (petCheckDoMenu) { petCheckDoMenu = false; appVoltarAoMenu(); return; }
  petView = PV_MAIN;
  petRedesenharSePreciso(true);
}

// Abre a tela de check com o primeiro pendente (ou sai, se não houver mais)
static void petAbrirCheck() {
  if (petAgenda.nPendentes == 0) { petSairParaOrigem(); return; }
  petView = PV_CHECK;
  petDrawCheck();
  mostrar();
}

// Grava a resposta no Google Calendar e só então muda a energia
static void petResponderCheck(bool feito) {
  // Aviso enquanto conversa com o Google (leva 1–3 s)
  gfx->fillRoundRect(40, 100, 160, 40, 10, COR(60, 40, 30));
  textoCentro(112, "Salvando...", 2, RGB565_WHITE);
  mostrar();

  const EventoPendente e = petAgenda.pendentes[0];
  if (!calendarioCheck(e.id, e.fim, feito)) {
    petVoltarParaCheck = true;
    petStartAction(cat_sad, COR(190, 205, 230), "Sem rede");
    mostrar();
    return;
  }

  // Confirmado no Google: aplica no gato
  petEnergy = clampStat(petEnergy + (feito ? AGENDA_ENERGIA_POR_CHECK : -AGENDA_ENERGIA_POR_CHECK));
  if (feito) petAgenda.sim++; else petAgenda.nao++;
  petHappy = humorDaSemana(petAgenda.sim, petAgenda.nao, petBonusHumor);

  // Tira o evento respondido da fila
  for (int i = 1; i < petAgenda.nPendentes; i++) petAgenda.pendentes[i - 1] = petAgenda.pendentes[i];
  petAgenda.nPendentes--;
  petSave();
  Serial.printf("Check: %s (energia %d)\n", feito ? "sim" : "nao", (int)petEnergy);

  petVoltarParaCheck = petAgenda.nPendentes > 0;
  if (feito) petStartAction(cat_happy, COR(200, 240, 190), "Boa!");
  else       petStartAction(cat_sad,   COR(190, 205, 230), "Tudo bem");
  mostrar();
}

// ─── Ações ───────────────────────────────────────────────────────────────────
static void petDoAction(int sel) {
  switch (sel) {
    case A_COMER:
      petHunger = clampStat(petHunger + PET_FEED_HUNGER);
      petSave();
      petStartAction(cat_eat, COR(255, 200, 140), "Nham!");
      break;
    case A_BRINCAR:
      petHappy  = clampStat(petHappy  + PET_PLAY_HAPPY);
      petEnergy = clampStat(petEnergy - PET_PLAY_ENERGY);
      petSave();
      petStartAction(cat_play, COR(255, 235, 120), "Yay!");
      break;
    case A_CARINHO:
      petHappy = clampStat(petHappy + PET_PET_HAPPY);
      petSave();
      petStartAction(cat_purr, COR(255, 200, 220), "Purr...");
      break;
  }
  mostrar();
}

// ─── Funções públicas ────────────────────────────────────────────────────────
void petBegin() {
  prefs.begin("lembregotchi", false);
  petLoad();
  petUltimoTick = millis();
  petUltimoSalvar = millis();
  petView = PV_MAIN;
}

// Tempo e agenda andam sempre; desenhar, só quando a tela é do gato (ativo)
void petLoop(bool ativo) {
  petAtivo = ativo;
  petTick();
  petSincronizarAgenda();
  if (!ativo) return;

  if (petView == PV_ACTION) {
    if ((int32_t)(millis() - petActionUntil) >= 0) {
      if (petVoltarParaCheck) {
        petVoltarParaCheck = false;
        petAbrirCheck();
      } else if (petVoltarAoMenu) {
        petVoltarAoMenu = false;
        appVoltarAoMenu();
      } else {
        petView = PV_MAIN;
        petRedesenharSePreciso(true);
      }
    }
    return;
  }

  if (petView == PV_MAIN) petRedesenharSePreciso(false);
}

void petAbrirGato() {
  petAtivo = true;
  petCheckDoMenu = false;
  petView = PV_MAIN;
  petRedesenharSePreciso(true);
}

void petAbrirAgenda() {
  petAtivo = true;
  petCheckDoMenu = true;
  if (petAgenda.nPendentes > 0) { petAbrirCheck(); return; }
  petVoltarAoMenu = true;           // nada pendente: comemora e volta ao menu
  petStartAction(cat_happy, COR(200, 240, 190), "Em dia!");
  mostrar();
}

void petAbrirStatus() {
  petAtivo = true;
  petCheckDoMenu = true;            // sai do Status para o menu
  petView = PV_STATS;
  petDrawStats();
  mostrar();
}

void petRedesenhar() {
  switch (petView) {
    case PV_STATS:  petDrawStats(); mostrar(); break;
    case PV_CHECK:  petDrawCheck(); mostrar(); break;
    case PV_ACTION:                      // a pose acabou enquanto a tela estava em descanso
      petView = PV_MAIN;
      petVoltarParaCheck = petVoltarAoMenu = false;
      petRedesenharSePreciso(true);
      break;
    default:        petRedesenharSePreciso(true); break;
  }
}

void petPlus() {
  if (petView == PV_ACTION) return;
  if (petView == PV_CHECK) { petResponderCheck(false); return; }   // ❌ não concluí
  if (petView == PV_STATS) { petSairParaOrigem(); return; }        // qualquer botão sai do Status
  petSel = (petSel + 1) % N_ACOES;
  petRedesenharSePreciso(true);
}

void petBoot() {
  if (petView == PV_ACTION) return;
  if (petView == PV_CHECK) { petResponderCheck(true); return; }    // ✅ concluí
  if (petView == PV_STATS) { petSairParaOrigem(); return; }
  petDoAction(petSel);
}

void petPwr() {
  if (petView == PV_ACTION) return;
  if (petView == PV_CHECK || petView == PV_STATS) { petSairParaOrigem(); return; }   // "depois"
  appVoltarAoMenu();                // do gato, PWR volta ao menu
}

// ─── Pomodoro e Home ─────────────────────────────────────────────────────────

// Recompensa: +1 de humor e +1 de energia por minuto focado (logica/pomodoro, máx. 50).
// Com agenda, o humor é recalculado pela semana a cada sincronização; por isso a parte do
// Pomodoro fica num bônus separado que soma por cima e vai caindo com o tempo.
void petRecompensar(int pontos) {
  petEnergy = clampStat(petEnergy + pontos);
  if (calendarioConfigurado()) {
    petBonusHumor = clampStat(petBonusHumor + pontos);
    petHappy = petAgendaHora ? humorDaSemana(petAgenda.sim, petAgenda.nao, petBonusHumor)
                             : clampStat(petHappy + pontos);
  } else {
    petHappy = clampStat(petHappy + pontos);
  }
  petSave();
  Serial.printf("Foco completo: +%d (humor %d, energia %d)\n", pontos, (int)petHappy, (int)petEnergy);
}

int petPendentes() { return petAgenda.nPendentes; }
int petEventosHoje() { return petAgendaHora ? petAgenda.hoje : -1; }
