#include "bloqueio.h"

bool deveBloquear(uint32_t agoraMs, uint32_t ultimaAtividadeMs, uint32_t limiteMs) {
  return agoraMs - ultimaAtividadeMs >= limiteMs;
}
