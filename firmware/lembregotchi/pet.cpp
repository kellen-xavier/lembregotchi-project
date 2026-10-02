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
#include "src/cat_sprites/cat_sprites.h"

extern Arduino_GFX *gfx;   // criado em lembregotchi.ino

// Converte r,g,b (0–255) para o formato de cor da tela (RGB565)
#define COR(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// ─── Estado ──────────────────────────────────────────────────────────────────
// Guardado como float para a queda lenta (ex.: 3 por hora) acumular a cada segundo.
static float    petHunger = PET_START_HUNGER;   // comida: 100 = de barriga cheia
static float    petHappy  = PET_START_HAPPY;    // humor
static float    petEnergy = PET_START_ENERGY;   // energia
static uint32_t petIdadeSeg = 0;                // tempo de vida contado com a placa ligada
static time_t   petNasc  = 0;                   // quando nasceu (hora real), 0 = ainda não sabe
static time_t   petVisto = 0;                   // última vez que o estado foi salvo (hora real)
static bool     petHoraAplicada = false;        // já descontou o tempo em que ficou desligado?

// Agenda
static time_t   petSyncDesde = 0;               // hora (do servidor) da última sincronização
static time_t   petAgendaInicio = 0;            // quando começou a acompanhar a agenda
static uint32_t petProximoSync = 0;             // millis() da próxima tentativa
static time_t   petAgendaHora = 0;              // quando sincronizou com sucesso pela última vez
static ResumoAgenda petAgenda = {};             // último resumo (os pendentes serão usados no passo 7)

enum PetView { PV_MAIN, PV_STATS, PV_ACTION };
static PetView  petView = PV_MAIN;
static int      petSel  = 0;           // 0 Comer, 1 Brincar, 2 Carinho, 3 Status
static uint32_t petActionUntil = 0;

static uint32_t petUltimoTick   = 0;
static uint32_t petUltimoSalvar = 0;
static int      petUltimoDesenho = -1; // "assinatura" do que está na tela

static const char *ACOES[4] = { "Comer", "Brincar", "Carinho", "Status" };

static Preferences prefs;

// ─── Utilidades ──────────────────────────────────────────────────────────────
static inline float clampStat(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

// Escreve um texto centralizado (a fonte padrão tem 6 px de largura por letra)
static void textoCentro(int y, const char *txt, int tamanho, uint16_t cor) {
  gfx->setTextSize(tamanho);
  gfx->setTextColor(cor);
  gfx->setCursor((TELA_W - (int)strlen(txt) * 6 * tamanho) / 2, y);
  gfx->print(txt);
}

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
}

// ─── Passagem do tempo ───────────────────────────────────────────────────────
static void aplicarHoras(float horas) {
  petHunger = clampStat(petHunger - PET_DECAY_HUNGER_PH   * horas);
  petHappy  = clampStat(petHappy  - PET_DECAY_HAPPY_PH    * horas);
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
// Humor:  taxa de conclusão (✅ / (✅ + ❌)) dos últimos 7 dias.
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

  int total = r.sim + r.nao;
  petHappy = total > 0 ? clampStat(100.0f * r.sim / total) : AGENDA_HUMOR_SEM_EVENTOS;

  petAgenda = r;
  petAgendaInicio = inicio;
  petAgendaHora = agoraEpoch();
  if (r.agora > 0) petSyncDesde = r.agora;   // próxima vez, só o que vier depois disto
  petSave();

  Serial.printf("Agenda: %d novos, semana %d sim / %d nao, %d pendentes\n",
                r.criados, r.sim, r.nao, r.nPendentes);
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

  // Ação escolhida, com setinhas dos lados
  gfx->fillRoundRect(30, 184, 180, 34, 10, tinta);
  gfx->fillTriangle(42, 201, 52, 193, 52, 209, fundo);     // ◀
  gfx->fillTriangle(198, 201, 188, 193, 188, 209, fundo);  // ▶
  textoCentro(194, ACOES[petSel], 2, fundo);

  // Bolinhas mostrando em qual das 4 ações estamos
  for (int i = 0; i < 4; i++) {
    int cx = TELA_W / 2 - 21 + i * 14;
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

// Mostra o resultado: o canvas é desenhado na memória e enviado de uma vez (sem piscar)
static void mostrar() {
  gfx->flush();
}

// Redesenha a tela principal só quando algo visível mudou
static void petRedesenharSePreciso(bool forcar) {
  // Junta humor, ação e os três valores (0–100) num número só para comparar
  int assinatura = (((int)petHumor() * 4 + petSel) * 101 + (int)petHunger) * 101 * 101
                   + (int)petHappy * 101 + (int)petEnergy;
  if (!forcar && assinatura == petUltimoDesenho) return;
  petUltimoDesenho = assinatura;
  petDrawMain();
  mostrar();
}

// ─── Ações ───────────────────────────────────────────────────────────────────
static void petDoAction(int sel) {
  switch (sel) {
    case 0:  // Comer
      petHunger = clampStat(petHunger + PET_FEED_HUNGER);
      petSave();
      petStartAction(cat_eat, COR(255, 200, 140), "Nham!");
      break;
    case 1:  // Brincar
      petHappy  = clampStat(petHappy  + PET_PLAY_HAPPY);
      petEnergy = clampStat(petEnergy - PET_PLAY_ENERGY);
      petSave();
      petStartAction(cat_play, COR(255, 235, 120), "Yay!");
      break;
    case 2:  // Carinho
      petHappy = clampStat(petHappy + PET_PET_HAPPY);
      petSave();
      petStartAction(cat_purr, COR(255, 200, 220), "Purr...");
      break;
    case 3:  // Status
      petView = PV_STATS;
      petDrawStats();
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
  petRedesenharSePreciso(true);
}

void petLoop() {
  petTick();
  petSincronizarAgenda();

  if (petView == PV_ACTION) {
    if ((int32_t)(millis() - petActionUntil) >= 0) {
      petView = PV_MAIN;
      petRedesenharSePreciso(true);
    }
    return;
  }

  if (petView == PV_MAIN) petRedesenharSePreciso(false);
}

void petPlus() {
  if (petView == PV_ACTION) return;
  if (petView == PV_STATS) {          // qualquer botão volta do Status
    petView = PV_MAIN;
    petRedesenharSePreciso(true);
    return;
  }
  petSel = (petSel + 1) % 4;
  petRedesenharSePreciso(true);
}

void petBoot() {
  if (petView == PV_ACTION) return;
  if (petView == PV_STATS) {
    petView = PV_MAIN;
    petRedesenharSePreciso(true);
    return;
  }
  petDoAction(petSel);
}
