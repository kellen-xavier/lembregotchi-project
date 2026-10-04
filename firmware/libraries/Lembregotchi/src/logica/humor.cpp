#include "humor.h"

float limitarStat(float v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

float humorDaSemana(int sim, int nao, float bonus) {
  if (sim < 0) sim = 0;
  if (nao < 0) nao = 0;
  int total = sim + nao;
  float base = total > 0 ? 100.0f * sim / total : HUMOR_SEM_EVENTOS;
  return limitarStat(base + bonus);
}
