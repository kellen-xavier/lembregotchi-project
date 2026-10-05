#pragma once
// Tela de descanso por inatividade. Lógica pura (testada em test_bloqueio.cpp).
#include <cstdint>

// true se ficou "limiteMs" sem tocar em nenhum botão (resiste à volta do millis())
bool deveBloquear(uint32_t agoraMs, uint32_t ultimaAtividadeMs, uint32_t limiteMs);
