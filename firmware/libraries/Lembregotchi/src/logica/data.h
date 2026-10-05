#pragma once
// Datas em português para a Home e a tela de descanso. Lógica pura (testada em test_data.cpp).
#include <cstddef>

// "04 OUT" — mes 1–12
void dataCurta(int dia, int mes, char *buf, size_t n);

// "Domingo 04 Out" — diaSemana 0 = domingo … 6 = sábado (como tm_wday); mes 1–12
void dataPorExtenso(int diaSemana, int dia, int mes, char *buf, size_t n);

// "HH:MM"
void horaMinuto(int hora, int minuto, char *buf, size_t n);
