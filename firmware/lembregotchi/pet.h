#pragma once

// Lembregotchi — bichinho virtual baseado no nekogotchi (MIT, Sebastian Berger),
// adaptado para a tela colorida 240x240 da Waveshare ESP32-S3 1.54" LCD.

void petBegin();   // carrega o estado salvo e desenha a tela inicial
void petLoop();    // chamar a cada volta do loop()
void petPlus();    // botão PLUS: próxima ação
void petBoot();    // botão BOOT: executa a ação escolhida (na tela "Concluiu?": sim)
void petPwr();     // botão PWR: na tela "Concluiu?", deixa para depois
