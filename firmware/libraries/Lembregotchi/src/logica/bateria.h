#pragma once
// Bateria: conversão do ADC e nível em %, pela tabela do firmware oficial da Waveshare
// (bsp_power_manager.c). Lógica pura (testada em test_bateria.cpp).
#include <cstdint>

// O divisor da placa entrega 1/3 da tensão da bateria no GPIO 1
uint32_t bateriaMvDoAdc(uint32_t adcMv);

// 1, 20, 40, 60, 80 ou 100 (%)
int bateriaNivel(uint32_t bateriaMv);
