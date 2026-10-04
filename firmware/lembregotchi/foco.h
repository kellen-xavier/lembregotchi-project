#pragma once
// Pomodoro ("Foco"): telas e botões. As regras do timer estão na biblioteca
// (logica/pomodoro.h) e são testadas em test/unit/test_pomodoro.cpp.

bool focoAtivo();                // alguma tela do Pomodoro está aberta?
void focoAbrir();                // tela inicial "Pomodoro"
void focoLoop(bool podeDesenhar);// avança o timer; redesenha se podeDesenhar
void focoRedesenhar();           // mostra de novo a tela atual (depois da pose de recompensa)
void focoPlus();
void focoBoot();
void focoPwr();
