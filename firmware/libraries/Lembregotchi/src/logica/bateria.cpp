#include "bateria.h"

uint32_t bateriaMvDoAdc(uint32_t adcMv) { return adcMv * 3; }

int bateriaNivel(uint32_t mv) {
  if (mv < 3520) return 1;
  if (mv < 3640) return 20;
  if (mv < 3760) return 40;
  if (mv < 3880) return 60;
  if (mv < 4000) return 80;
  return 100;
}
