#pragma once
// Humor com agenda: taxa de conclusão da semana + bônus (pomodoro, carinho…). Lógica pura.
// Testada em test/unit/test_humor.cpp.

#define HUMOR_SEM_EVENTOS 50   // neutro quando não houve eventos na semana

// 100 × sim / (sim + nao), ou HUMOR_SEM_EVENTOS se não houve eventos; + bônus; limitado a 0–100
float humorDaSemana(int sim, int nao, float bonus);

// Limita um valor do gato (comida, humor, energia) a 0–100
float limitarStat(float v);
