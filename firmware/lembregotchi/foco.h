#pragma once
// Pomodoro clássico ("Foco"): telas e botões. As regras do timer estão na biblioteca
// (logica/pomodoro.h) e são testadas em test/unit/test_pomodoro.cpp.

bool focoRodando();               // timer ligado (foco ou descanso)? Então não entra na tela de descanso
void focoAbrir();                 // menu → Pomodoro (se o timer estiver rodando, volta para ele)
void focoLoop(bool podeDesenhar); // avança o timer; redesenha se podeDesenhar
void focoRedesenhar();            // mostra de novo a tela atual
void focoPlus();
void focoBoot();
void focoPwr();
void focoLongo();                 // clique longo: "Tem certeza?" (sair do Pomodoro)
