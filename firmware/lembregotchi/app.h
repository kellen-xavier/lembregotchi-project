#pragma once
// Navegação do aparelho (estilo Pala Note):
//
//   Home (data, bateria, tarefas, eventos) ──BOOT──► menu: Gato · Agenda · Status · Pomodoro
//   Sem tocar em botão por 1 min → tela de descanso (relógio, luz baixa); qualquer botão volta.

void appBegin();
void appLoop();
void appPlus();
void appBoot();
void appPwr();
void appLongo();          // clique longo (qualquer botão)
void appVoltarAoMenu();   // chamado pelas telas (pet, Pomodoro) ao sair
