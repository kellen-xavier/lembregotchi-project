#pragma once

// Lembregotchi — bichinho virtual baseado no nekogotchi (MIT, Sebastian Berger),
// adaptado para a tela colorida 240x240 da Waveshare ESP32-S3 1.54" LCD.
// As telas são abertas pelo menu (app.cpp); ao sair, o pet chama appVoltarAoMenu().

void petBegin();             // carrega o estado salvo
void petLoop(bool ativo);    // a cada volta do loop(); ativo = a tela do pet está na frente
void petRedesenhar();        // desenha de novo a tela atual (ex.: ao sair da tela de descanso)

void petAbrirGato();         // menu → Gato (ações Comer/Brincar/Carinho)
void petAbrirAgenda();       // menu → Agenda ("Concluiu?" ou "Em dia!")
void petAbrirStatus();       // menu → Status

void petPlus();              // próxima ação / "não concluí" / sai do Status
void petBoot();              // executa / "concluí"
void petPwr();               // volta (ao menu) / "depois"

void petRecompensar(int pontos);   // Pomodoro: foco completo → humor e energia
int  petPendentes();               // eventos esperando "Concluiu?" (Home: TAREFAS)
int  petEventosHoje();             // eventos de hoje ainda por vir (Home: EVENTOS); -1 = sem dados
