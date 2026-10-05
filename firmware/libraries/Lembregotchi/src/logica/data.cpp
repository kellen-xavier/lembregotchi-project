#include "data.h"

#include <cstdio>

// Sem acentos: a fonte da tela só tem ASCII
static const char *DIAS[7]   = { "Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado" };
static const char *MESES[12] = { "Jan", "Fev", "Mar", "Abr", "Mai", "Jun",
                                 "Jul", "Ago", "Set", "Out", "Nov", "Dez" };
static const char *MESES_MAI[12] = { "JAN", "FEV", "MAR", "ABR", "MAI", "JUN",
                                     "JUL", "AGO", "SET", "OUT", "NOV", "DEZ" };

static int limitar(int v, int min, int max) { return v < min ? min : (v > max ? max : v); }

void dataCurta(int dia, int mes, char *buf, size_t n) {
  snprintf(buf, n, "%02d %s", limitar(dia, 0, 99), MESES_MAI[limitar(mes, 1, 12) - 1]);
}

void dataPorExtenso(int diaSemana, int dia, int mes, char *buf, size_t n) {
  snprintf(buf, n, "%s %02d %s", DIAS[limitar(diaSemana, 0, 6)], limitar(dia, 0, 99),
           MESES[limitar(mes, 1, 12) - 1]);
}

void horaMinuto(int hora, int minuto, char *buf, size_t n) {
  snprintf(buf, n, "%02d:%02d", limitar(hora, 0, 99), limitar(minuto, 0, 99));
}
