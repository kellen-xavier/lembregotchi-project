# AGENTS.md — Lembregotchi

Instruções para agentes de IA (e pessoas) que trabalham neste repositório.

## O que é

**Lembregotchi**: um bichinho virtual (gato) para a placa **Waveshare ESP32-S3 1.54" LCD** cujo estado vem
do **Google Calendar** da dona:

| Valor | Sobe | Cai |
|---|---|---|
| Comida | +20 por evento novo criado na agenda | passagem do tempo (−3/h) |
| Energia | check "sim" no aparelho (+15) | check "não" (−15) |
| Humor | taxa de conclusão (✅ ÷ (✅ + ❌)) dos últimos 7 dias | — |

O desenho e a lógica do gato vêm do [nekogotchi](https://github.com/defcon1702/pala-nekogotchi) (MIT),
adaptados de e-paper 1-bit para LCD colorido.

## ⚠️ O repositório é PÚBLICO

- **Nunca** commitar, imprimir, logar ou colar em chat: senha de Wi-Fi, URL do Apps Script, chave da ponte.
  Tudo isso vive só em `firmware/lembregotchi/segredos.h`, que está no `.gitignore`.
- Para verificar segredos, confira **só tamanho/formato**, nunca o valor:
  ```bash
  sed -nE 's/^#define[[:space:]]+WIFI_SSID[[:space:]]+"([^"]*)".*/\1/p' firmware/lembregotchi/segredos.h | awk '{print length}'
  ```
- A serial **não** pode mostrar a chave nem o corpo das respostas da agenda (têm títulos de eventos).
- Antes de qualquer commit: `git status`, confira se `segredos.h` não aparece e faça uma varredura
  (ex.: `grep -rnE "script.google.com/macros/s/[A-Za-z0-9_-]{10,}|[0-9a-f]{64}"` nos arquivos novos).
- HTTPS **sempre** com verificação de certificado (`certificados.h`). Nunca usar `setInsecure()`.
- Não adicionar material com direitos autorais de terceiros (datasheets etc.). Use links.
  Exceção decidida pela dona: a imagem padrão do Pomodoro (`image/descansa-guerreiro.jpeg`,
  brincadeira compartilhada publicamente pela FromSoftware). A imagem é **trocável**
  (`make imagem IMG=…`); o código nunca depende dela, só do símbolo `img_foco`.

## Estrutura

```
AGENTS.md                      este arquivo
apps-script/                   ponte Google Calendar (roda na conta Google da dona)
  Codigo.gs                    doPost: "resumo" e "check"; chave em Propriedades do script
  appsscript.json              manifesto: só o escopo de agenda
  testar.sh                    testa a ponte lendo URL/chave do segredos.h (sem imprimi-los)
docs/
  arquitetura.md               hardware: componentes, mapa de GPIOs, I2C, libs, linguagem
  integracao-google-calendar.md  regras e desenho da integração (decisões)
  tutorial-my-board.md         tutorial didático, passo a passo (1 → 7)
firmware/
  libraries/Lembregotchi/      BIBLIOTECA DO PROJETO — usada pelo firmware, testes de placa e unitários
    library.properties
    src/Lembregotchi.h         inclua só este: #include <Lembregotchi.h>
    src/placa/placa.h/.cpp     pinos (LCD_*, BTN_*, BAT_POWER_HOLD, TELA_*), placaIniciar/NovaTela/Luz
    src/logica/                LÓGICA PURA (sem Arduino) — roda no PC e é testada
      texto.h / texto.cpp      asciiSimples()
      humor.h / humor.cpp      humorDaSemana(sim, nao, bonus), limitarStat()
      pomodoro.h / pomodoro.cpp  timer do Pomodoro (fases, pausa, recompensa, MM:SS)
  lembregotchi/                firmware principal (sketch Arduino)
    lembregotchi.ino           setup/loop, tela (Canvas), botões
    tela.h / tela.cpp          COR(), cores comuns, textoCentro(), mostrar()
    foco.h / foco.cpp          telas e botões do Pomodoro ("Foco")
    config.h                   constantes do bichinho, rede e agenda (pinos vêm da biblioteca)
    pet.h / pet.cpp            estado, tempo, humor, desenho, ações, tela "Concluiu?"
    rede.h / rede.cpp          Wi-Fi + hora (NTP, fuso de Brasília)
    calendario.h / .cpp        cliente HTTPS da ponte (ArduinoJson)
    certificados.h             raízes GTS R1/R4 do Google (públicas)
    segredos.exemplo.h         modelo vazio (versionado)
    segredos.h                 valores reais (IGNORADO pelo git)
    src/cat_sprites/           8 sprites 150×150 1-bit + LICENSE (MIT, nekogotchi)
    src/imagens/foco.h         GERADO por `make imagem` (img_foco, 220×150 RGB565) — não editar
    image/                     imagens originais (padrão: descansa-guerreiro.jpeg)
test/
  unit/                        testes unitários da lógica pura (doctest); um test_<modulo>.cpp por módulo
    main.cpp                   gera o main() do doctest
  placa/                       testes mínimos NA PLACA (sketches), usando a mesma biblioteca
    passo1_tela/               tela: cores + desenho
    passo2_botoes/             botões: clique, duplo, longo
  apps-script/codigo.test.mjs  testes da ponte (node:test) com Google falso (vm + CalendarApp simulado)
  vendor/doctest.h             framework de testes (MIT, v2.4.12) + LICENSE-doctest.txt
  tools/test_imagem_rgb565.py  testes do conversor de imagem (Python unittest)
tools/imagem_rgb565.py         converte JPG/PNG em .h RGB565 (recorte central ou CORTE manual)
Makefile                       make test | compilar | gravar | gravar-teste T=… | ide | imagem IMG=…
compile_flags.txt              caminhos de include para o clangd (editor)
```

> A biblioteca é achada pelo `arduino-cli` com `--libraries firmware/libraries` (o `Makefile` já
> passa). Para a Arduino IDE: `make ide` cria o link `~/Arduino/libraries/Lembregotchi`.

> Na Arduino IDE, só a pasta `src/` de um sketch é compilada junto; por isso os sprites ficam lá.

## Hardware (resumo — detalhes em `docs/arquitetura.md`)

- ESP32-S3R8 · 16 MB flash · 8 MB PSRAM · USB-Serial/JTAG nativo → `/dev/ttyACM0` (ID `303a:1001`)
- Tela ST7789 240×240 (SPI): DC 45 · CS 21 · SCK 38 · MOSI 39 · RST 40 · BL 46
- Botões (ativos em LOW): BOOT 0 · PLUS 4 · PWR 5
- GPIO 2 = manter ligada na bateria (HIGH logo no início do `setup()`)
- **Sem RTC**: hora vem de NTP. **Só Wi-Fi 2,4 GHz.**

## Compilar e gravar

Toolchain: `arduino-cli` embutido na Arduino IDE, usando a configuração da IDE.

```bash
CLI=/opt/arduino-ide/resources/app/lib/backend/resources/arduino-cli
CFG=$HOME/.arduinoIDE/arduino-cli.yaml
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"

$CLI --config-file $CFG compile --fqbn $FQBN firmware/lembregotchi
$CLI --config-file $CFG upload  -p /dev/ttyACM0 --fqbn $FQBN firmware/lembregotchi
```

| Dependência | Versão |
|---|---|
| esp32 by Espressif (core) | 3.3.12 |
| GFX Library for Arduino | 1.6.8 (**a 1.6.0 não compila** com o core 3.3.x) |
| OneButton | 2.6.2 |
| ArduinoJson | 7.4.3 |

- Permissão da porta: o usuário precisa estar no grupo `uucp` (Arch). Temporário:
  `sudo setfacl -m u:$USER:rw /dev/ttyACM0` (some ao reconectar a placa).
- Ler a serial **sem** resetar a placa: abrir com `dtr=False, rts=False` (pyserial). A USB
  reenumera no reset; tolere a porta sumir por ~1 s.
- `Failed to connect`: segurar BOOT, apertar RST, soltar BOOT.

## Apps Script (ponte)

- Contrato (POST JSON, sempre com `chave`):
  - `{"acao":"resumo","desde":<epoch>,"inicio":<epoch>}` → `{ok, agora, criados, pendentes[{id,titulo,fim}], semana{sim,nao}}`
  - `{"acao":"check","id":"…","fim":<epoch>,"feito":true|false}` → grava ✅/❌ no início do título
- `inicio` = primeira sincronização do aparelho; eventos que terminaram antes são ignorados.
- O Apps Script responde **302** para `script.googleusercontent.com`; o firmware segue à mão com GET
  e **só** para esse domínio.
- Respostas vêm **chunked**: ler com `http.getString()`, **não** `getStream()`.
- Toda resposta traz `"versao"` (constante `VERSAO` no topo do `Codigo.gs`). **A cada mudança no
  `Codigo.gs`, incremente `VERSAO`** (formato `AAAA-MM-DD.N`).
- Após mudar `Codigo.gs`, a dona precisa **colar no editor e reimplantar** (Gerenciar implantações →
  ✏️ → *Nova versão*). Confira com `apps-script/testar.sh`: ele compara a versão no ar com a local
  (`✅ versão implantada = local`). `testar.sh sem-chave` → `nao autorizado`.
- Validar sintaxe localmente: copiar para `.js` e `node --check`.

## Convenções

- **Idioma**: código, comentários, nomes e docs em **português** (sem acentos em strings exibidas na
  tela — a fonte é ASCII; use `asciiSimples()` para textos vindos da internet).
- Estilo: igual ao código existente — `static` para tudo interno de cada `.cpp`, prefixo `pet…`
  em `pet.cpp`, constantes em `config.h`, comentários curtos explicando o *porquê*.
- Desenho: sempre no `gfx` (Canvas em memória) e `gfx->flush()` uma vez por quadro, para não piscar.
  Fonte padrão: 6 px de largura × tamanho por letra; centralize com `textoCentro()`.
- O estado do gato fica em `Preferences` (namespace `lembregotchi`). Ao adicionar campo, salve em
  `petSave()` e leia em `petLoad()` com valor padrão.
- Mudança no gato que depende da agenda: **aplicar só depois que o Google confirmar**.
- **Editar, não sobrescrever**: arquivos existentes (inclusive `.gitignore` e docs) devem ser
  editados pontualmente ou receber acréscimos; nunca regenerados inteiros. A dona edita arquivos
  entre as sessões.
- Toda etapa nova ganha uma seção didática em `docs/tutorial-my-board.md` (o que é, por que, como
  testar, tabela de diagnóstico).
- Não fazer commit/push sem pedido explícito.

## Testes unitários (obrigatórios)

**Toda funcionalidade deve ter teste.** Para isso, a lógica é separada do hardware:

| Camada | Onde | Pode usar | Como testa |
|---|---|---|---|
| Lógica pura | `firmware/libraries/Lembregotchi/src/logica/` | só C++ padrão (`<cstring>`, `<cstdint>`…) | `test/unit/` — `make test` no PC |
| Placa | `src/placa/` da biblioteca | Arduino, pinos | `test/placa/` — `make gravar-teste T=…` + foto/serial |
| Firmware | `pet.cpp`, `rede.cpp`, `calendario.cpp`, `.ino` | Arduino, tela, Wi-Fi, `millis()` | na placa (serial/foto) |

- **Todo teste usa a mesma biblioteca** (`<Lembregotchi.h>` / `logica/…`): nada de copiar pinos ou
  funções para dentro de um teste. Testes unitários C++ usam **doctest**.
- Regra nova ou cálculo novo → vai para `src/logica/` da biblioteca (sem `Arduino.h`, sem `millis()`, sem `gfx`);
  o código de hardware só chama essas funções. Dados de entrada (hora, valores) entram por parâmetro.
- Cada módulo `src/logica/X.cpp` tem `test/unit/test_X.cpp`. O `Makefile` pega tudo sozinho.
- Teste casos normais **e** de borda (vazio, limite, buffer pequeno, valor fora da faixa).
- `make test` compila com `-Wall -Wextra -Werror` e **ASan/UBSan**: erro de memória reprova o teste.
- Antes de entregar: `make test` verde **e** `make compilar` (firmware + testes de placa).

## Testar

| O quê | Como |
|---|---|
| Compila | comando `compile` acima |
| Inicia sem travar | serial: `Lembregotchi pronto!` |
| Wi-Fi / hora | serial: `Wi-Fi: conectado`, `Hora: sincronizada` |
| Agenda | serial: `Agenda: N novos, semana S sim / N nao, P pendentes` |
| Ponte (pelo PC) | `apps-script/testar.sh` |
| Lógica pura (`src/logica/`) | `make test` |
| Visual | pedir foto da tela à dona — o agente não vê o display |

Diagnóstico de Wi-Fi: `WiFi.onEvent` com `ARDUINO_EVENT_WIFI_STA_DISCONNECTED` dá o motivo
(`NO_AP_FOUND` = rede não vista/5 GHz/nome diferente; `4WAY_HANDSHAKE_TIMEOUT` = senha errada).

## Referências

- Manual da placa: https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54
- Esquema elétrico: https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.54/ESP32-S3-LCD-1.54-Schematic.pdf
- Exemplos oficiais Waveshare: https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54
- Arduino-ESP32 Wi-Fi API: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html
- Arduino_GFX: https://github.com/moononournation/Arduino_GFX
- OneButton: https://github.com/mathertel/OneButton
- ArduinoJson: https://arduinojson.org/
- Google Apps Script — CalendarApp: https://developers.google.com/apps-script/reference/calendar
- nekogotchi (origem do gato, MIT): https://github.com/defcon1702/pala-nekogotchi

## Estado atual (2026-10-04)

- Implementados: tela, botões, gato, Wi-Fi/NTP, ponte, sincronização, "Concluiu?" e
  **Pomodoro** (Foco Baixo 25+5, Guerreiro 50+10, tempo livre 5–60).
- **Tags (#hashtag) foram REMOVIDAS** a pedido da dona (2026-10-04), em todas as camadas
  (firmware, biblioteca, ponte, testes). O código completo está no commit `e778b15`
  (`git show e778b15`) para quando forem retomadas. Não reintroduzir sem pedido.
- Ações do gato: Comer, Brincar, Carinho, Agenda, Foco, Status.
- Recompensa do Pomodoro: +1 humor/energia por minuto (máx. 50); humor vai para `petBonusHumor`
  (somado à taxa da semana, cai −2/h) para não ser apagado pela sincronização.
- Ponte local `2026-10-04.1` (sem tags). No ar ainda está a `2026-10-03.1` (com tags) — funciona
  com o firmware (campos extras são ignorados); reimplantar para o `testar.sh` aprovar.
- Ponte `2026-10-03.1` foi implantada e confirmada (`testar.sh`: versão = local + chave aceita);
  placa sincronizando. Havia 2 implantações; usar só a do `segredos.h` (arquivar as antigas).
- `testar.sh` só aprova com `ok:true` + versão igual; `sem-chave` exige `nao autorizado`.
- `LEMBREGOTCHI_CHAVE` = 64 hex do `openssl rand -hex 32` (≠ ID da implantação, que tem 72 e começa com `AKfy`).
- A testar na placa: telas do Pomodoro (pedir foto).
- Testes: C++ cobre texto, humor, pomodoro; ponte cobre inicio, pendentes, criados,
  chave, versão, check. Ainda sem teste (lógica dentro do `pet.cpp`): passagem do tempo, tempo
  desligado, humor → desenho, efeitos das ações, quebra do título em linhas.
- Ideias futuras: configurar Wi-Fi pelo celular via `WiFi.softAP` (sem senha no código),
  `WiFiMulti` para várias redes, sons pelo ES8311.
