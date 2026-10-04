
---

## Passo 3 — O gato na tela (lógica do nekogotchi)

Código: `firmware/lembregotchi/` (abra o `lembregotchi.ino` na Arduino IDE; os outros arquivos
abrem juntos como abas).

### Organização dos arquivos

| Arquivo | O que faz |
| --- | --- |
| `lembregotchi.ino` | liga a tela e os botões, e chama o bichinho no `loop()` |
| `config.h` | pinos da placa e números do bichinho (quanto a fome cai por hora etc.) |
| `pet.h` / `pet.cpp` | o bichinho: estado, passagem do tempo, humor, desenho e ações |
| `src/cat_sprites/` | os 8 desenhos do gato (do nekogotchi, licença MIT) |

> Na Arduino IDE, só a pasta `src/` é compilada junto com o sketch. Por isso os desenhos ficam ali.

### Como funciona

1. Três números de 0 a 100

| Valor | Sobe com | Cai com |
| --- | --- | --- |
| **Comida** | Comer (+50) | o tempo (−3 por hora) |
| **Humor** | Brincar (+20), Carinho (+10) | o tempo (−2 por hora) |
| **Energia** | o tempo / descanso (+8 por hora) | Brincar (−15) |

**2. Humor → desenho e cor de fundo** (o primeiro que valer, de cima para baixo)

| Condição | Desenho | Fundo |
| --- | --- | --- |
| energia < 20 | dormindo | azul-escuro |
| humor < 15 **e** comida < 15 | triste | azul-acinzentado |
| comida < 30 | com fome | laranja-claro |
| comida > 70 **e** humor > 70 | feliz | verde-claro |
| senão | contente | creme |

3. Os desenhos

Cada desenho tem 150×150 pixels de **1 bit** (cada pixel é "pinta" ou "não pinta"). Cada linha
ocupa 19 bytes (150 bits arredondados para cima). É o mesmo formato que a função
`gfx->drawBitmap()` entende. Na tela colorida, escolhemos a **cor da tinta** e a **cor do fundo**,
por isso o mesmo desenho pode ficar de cores diferentes conforme o humor.

4. Canvas: desenhar sem piscar

Em vez de desenhar direto na tela (o que faz a imagem piscar enquanto é montada), o programa
desenha numa **"folha" na memória** (o *canvas*, 240×240×2 bytes ≈ 113 KB) e depois manda tudo de
uma vez com `gfx->flush()`.

**5. O tempo sem relógio**

A placa **não tem RTC**, então o tempo é contado com `millis()` (milissegundos desde que ligou).
A cada segundo, o programa aplica a parte proporcional da queda/subida.
**Por enquanto, o tempo com a placa desligada não conta.** Isso será resolvido no passo 4
(hora pela internet).

Para testar mais rápido, mude em `config.h`:

```c
#define PET_VELOCIDADE 60   // 1 minuto = 1 hora do bichinho
```

6. Salvar na memória (Preferences)

O estado é salvo na flash interna (biblioteca `Preferences`, que já vem com o ESP32) a cada ação e
a cada 5 minutos. Ao religar, o gato volta como estava.

### Controles

| Botão | Ação |
| --- | --- |
| **PLUS** | escolhe a próxima ação: Comer → Brincar → Carinho → Status |
| **BOOT** | executa a ação escolhida (a pose aparece por ~1,4 s) |
| qualquer um, na tela Status | volta |

### Tela principal

```txt
┌────────────────────────────┐
│ [comida] [humor] [energia] │  ← barrinhas laranja, amarela e azul
│                            │
│         (gato 150×150)     │
│                            │
│     ◀    Comer    ▶        │  ← ação escolhida
│          ● ○ ○ ○           │  ← qual das 4 ações
└────────────────────────────┘
```

---

## Passo 4 — Wi-Fi e hora real

> A partir daqui o Lembregotchi passa a ser ligado ao **Google Calendar**. O desenho completo está
> em `docs/integracao-google-calendar.md`. O passo 4 é a base: sem internet e hora certa, não dá
> para conversar com a agenda.

### Por que a hora importa

A placa **não tem relógio** (RTC). Quando desliga, ela "esquece" que horas são. Sem a hora real:

- o tempo com a placa desligada não conta (o gato nunca sente fome enquanto está guardado);
- não dá para saber quando um evento da agenda terminou.

### NTP: perguntar a hora para a internet

**NTP** (*Network Time Protocol*) são servidores públicos que respondem "que horas são" com precisão
de milissegundos. O ESP32 já sabe falar NTP:

```cpp
configTzTime("<-03>3", "pool.ntp.org", "time.google.com");   // fuso de Brasília
time_t agora = time(nullptr);   // segundos desde 01/01/1970 (o "epoch")
```

Antes de sincronizar, o relógio interno começa em 1970. Por isso o programa só confia na hora se
ela for depois de 2023.

### Como o gato desconta o tempo desligado

O programa guarda na memória a **hora da última vez que salvou** (`visto`). Ao ligar e descobrir a
hora real:

```txt
  visto ─────── desligado ─────── ligou ──── ligado (millis) ──── agora
  (salvo)        (descontar)                 (já contado)
```

- `ligou = agora − millis()/1000`: o tempo com a placa ligada já foi contado pelo `millis()`;
- `desligado = ligou − visto`, limitado a **48 h** (como no nekogotchi);
- a comida e o humor caem, e a energia sobe, proporcionalmente a esse tempo.

A **idade** passa a ser contada desde o "nascimento" real (`nasc`), mesmo com a placa desligada.

### Segredos fora do código

O nome e a senha do Wi-Fi ficam em `segredos.h`, um arquivo separado:

- `segredos.exemplo.h`: modelo vazio, que pode ser compartilhado;
- `segredos.h`: o seu, preenchido; está no `.gitignore` para **nunca** ir para o GitHub.

```c
#define WIFI_SSID  "NomeDaRede"
#define WIFI_SENHA "senha"
```

> O ESP32 só conecta em redes **2,4 GHz** (não em 5 GHz).

### Como saber se deu certo

| Onde | O que aparece |
| --- | --- |
| Ao ligar | "Conectando..." por alguns segundos |
| Tela **Status** | `Wi-Fi ok  21:40` (verde) com a hora certa |
| Serial Monitor | `Wi-Fi: conectado, IP …` e `Hora: sincronizada` |
| Status sem rede | `Sem Wi-Fi` (vermelho); o gato continua funcionando |

---

## Passo 5 — A ponte com o Google Calendar (Apps Script)

### O que é o Apps Script

O **Google Apps Script** é um lugar para rodar pequenos programas em JavaScript **dentro da sua
conta Google**, de graça. Ele já tem acesso fácil à sua agenda (`CalendarApp`). Publicado como
**"app da web"**, ele ganha uma URL que o ESP32 consegue chamar.

```txt
ESP32 ──POST {chave, acao}──► Apps Script ──► sua agenda
      ◄──── JSON pequeno ────
```

Assim, o ESP32 **não faz login no Google** e **não guarda senha do Google**. Ele só conhece a URL e
uma chave.

Arquivos (públicos, **sem segredos**):

| Arquivo | O que é |
| --- | --- |
| `apps-script/Codigo.gs` | o código da ponte |
| `apps-script/appsscript.json` | o "manifesto": fuso horário e a **única** permissão pedida (agenda) |
| `apps-script/testar.sh` | testa a ponte pelo terminal, lendo URL e chave do `segredos.h` |

### O que a ponte responde

**`resumo`**: o aparelho pergunta "o que mudou desde a última vez?" e recebe:

```json
{ "ok": true, "agora": 1759370000, "criados": 2,
  "pendentes": [ { "id": "…", "titulo": "Reuniao X", "fim": 1759366800 } ],
  "semana": { "sim": 5, "nao": 2 } }
```

- `criados`: eventos novos → **Comida**;
- `pendentes`: eventos que terminaram nas últimas 24 h sem ✅/❌ → o gato pergunta "Concluiu?";
- `semana`: ✅ e ❌ dos últimos 7 dias → **Humor**.

**`check`**: grava ✅ ou ❌ no início do título do evento.

### 🔒 Segurança (o repositório é público)

| Risco | Proteção |
| --- | --- |
| Senha do Wi-Fi, URL ou chave vazarem no GitHub | ficam **só** no `segredos.h`, que está no `.gitignore` e nunca foi commitado |
| A chave estar no código do Apps Script | **não está**: fica em *Propriedades do script*, dentro da sua conta |
| Alguém achar a URL e ler a sua agenda | toda chamada exige a **chave** (64 caracteres aleatórios); sem ela, só recebe `nao autorizado` |
| Adivinhar a chave aos poucos | comparação em **tempo constante** + chave longa demais para tentativa e erro |
| Chave aparecer em históricos e logs de URL | a chave vai no **corpo do POST**, nunca na URL; o GET não devolve nada |
| A ponte mexer em qualquer evento | `check` só altera eventos **já terminados nos últimos 7 dias**, que batem com id **e** horário, e só acrescenta ✅/❌ no título |
| Vazar dados demais | a resposta traz só contagens e no máximo 5 títulos curtos (40 letras) |
| Permissões demais | o manifesto pede só a permissão de **agenda**, e o script usa só a **agenda principal** |
| Mensagens de erro revelarem detalhes | erros internos respondem só `erro interno` |

**Se a chave vazar:** gere outra, troque em *Propriedades do script* **e** no `segredos.h`. A antiga
para de funcionar na hora. Para desligar a ponte de vez: *Implantar → Gerenciar implantações →
Arquivar*.

> Nunca cole a chave ou a URL em issues, prints, commits ou chats.

### Como instalar (uma vez)

**1. Criar o projeto**

1. Acesse https://script.google.com e clique em **Novo projeto**. Dê o nome `Lembregotchi`.
2. Apague o conteúdo do `Código.gs` e cole o conteúdo de `apps-script/Codigo.gs`.
3. Em **⚙️ Configurações do projeto**, marque **"Mostrar arquivo de manifesto appsscript.json"**.
   Volte ao editor, abra o `appsscript.json` e cole o conteúdo de `apps-script/appsscript.json`.
4. Salve (Ctrl+S).

2. Gerar a chave (no SEU terminal, não no chat)

Este comando gera 64 caracteres aleatórios e grava direto no `segredos.h`:

```bash
cd ~/Projects/lembregotchi-project
CHAVE=$(openssl rand -hex 32)
sed -i "s|^#define LEMBREGOTCHI_CHAVE .*|#define LEMBREGOTCHI_CHAVE \"$CHAVE\"|" firmware/lembregotchi/segredos.h
echo "$CHAVE"   # copie para o próximo passo
unset CHAVE
```

3. Guardar a chave no Apps Script

1. Em **⚙️ Configurações do projeto → Propriedades do script → Adicionar propriedade**:
   - Propriedade: `LEMBREGOTCHI_CHAVE`
   - Valor: a chave copiada

2. Salve. No editor, escolha a função **`testarChaveConfigurada`** e clique em **Executar**.
   - Na primeira execução, o Google pede autorização: **Revisar permissões** → escolha sua conta →
     "O Google não verificou este app" → **Avançado** → **Acessar Lembregotchi (não seguro)** →
     **Permitir**. Esse aviso aparece porque o script é seu e não passou pela verificação do Google.
   - O log deve mostrar `Chave configurada (64 caracteres)`.

4. Testar a leitura da agenda (pelo editor)

- Execute **`testarResumo`**. O log mostra o JSON com os seus eventos do último dia.

5. Publicar como app da web

1. **Implantar → Nova implantação →** ⚙️ tipo **App da Web**.
2. **Executar como:** *Eu* · **Quem pode acessar:** *Qualquer pessoa*.
   - "Qualquer pessoa" é necessário porque o ESP32 não consegue fazer login no Google. A proteção é a **chave**.
3. Clique em **Implantar** e copie a **URL do app da web** (termina em `/exec`).
4. Cole no `segredos.h`:

   ```c++
   #define LEMBREGOTCHI_URL "https://script.google.com/macros/s/…/exec"
   ```

> Sempre que alterar o `Codigo.gs`, faça **Implantar → Gerenciar implantações → ✏️ → Versão: Nova
> versão**, para manter a mesma URL.

6. Testar pelo terminal

```bash
apps-script/testar.sh             # deve responder {"ok":true, ...}
apps-script/testar.sh sem-chave   # deve responder {"ok":false,"erro":"nao autorizado"}
```

---

## Passo 6 — O ESP32 busca a agenda

Código novo: `firmware/lembregotchi/calendario.cpp` e `certificados.h`. Biblioteca nova: **ArduinoJson 7.4.3**.

### O que acontece a cada 5 minutos

```txt
ESP32 ──POST {chave, "resumo", desde}──► Apps Script
      ◄── {criados, semana:{sim,nao}, pendentes[]} ──
```

| Dado recebido | Efeito no gato |
| --- | --- |
| `criados` | **Comida** +20 por evento novo |
| `semana.sim` / `semana.nao` | **Humor** = 100 × sim ÷ (sim + não); sem eventos = 50 |
| `pendentes` | guardados para a tela "Concluiu?" (passo 7) |

- `desde` é a hora da última sincronização bem-sucedida, guardada na flash. Assim, cada evento
  novo alimenta o gato **uma vez só**. Na primeira sincronização, `desde = 0` e nada conta (para
  não "alimentar" com a agenda inteira).
- Com a agenda configurada, a **energia** quase não se recupera sozinha (+2/h em vez de +8/h):
  ela passa a vir dos checks.
- Se falhar (sem rede, Google fora), tenta de novo em 1 minuto. O gato continua funcionando.

### 🔒 HTTPS de verdade: verificando o certificado

Uma conexão HTTPS só é segura se o aparelho **confere quem está do outro lado**. Muitos exemplos
na internet usam `client.setInsecure()`, que aceita **qualquer** servidor: alguém na sua rede
poderia se passar pelo Google e roubar a chave.

Aqui o ESP32 só aceita servidores assinados pelas **raízes do Google** (GTS Root R1 e R4, válidas
até 2036), guardadas em `certificados.h`. Certificados raiz são **públicos**, então podem ficar no
repositório sem problema.

```cpp
WiFiClientSecure cliente;
cliente.setCACert(GOOGLE_ROOT_CA);   // só confia no Google
```

### O redirecionamento do Apps Script

O Apps Script sempre responde ao POST com um **redirecionamento** (código 302) para
`script.googleusercontent.com`, onde fica a resposta de verdade. O ESP32 segue esse
redirecionamento **à mão**, e **só** se o destino for mesmo esse domínio do Google.

### Tela Status

| Linha | Significado |
| --- | --- |
| `Agenda ok  2 pend.` (verde) | sincronizou nos últimos 30 min; 2 eventos aguardando check |
| `Agenda sem sinal` (vermelho) | configurada, mas não conseguiu falar com a ponte |
| `Agenda nao config.` | `LEMBREGOTCHI_URL` / `LEMBREGOTCHI_CHAVE` vazios no `segredos.h` |

### Problema comum: Wi-Fi não conecta

O ESP32 **só enxerga redes de 2,4 GHz**. Muitos roteadores têm duas redes (ex.: `Casa` e
`Casa_5G`), ou uma só com "band steering", que junta as duas.

- Use o nome da rede **2,4 GHz**, exatamente igual (maiúsculas e minúsculas contam).
- Se o roteador junta as bandas num nome só, ative uma rede 2,4 GHz separada nas configurações dele.

---

## Passo 7 — "Concluiu?": o check no aparelho

### Quando aparece

- **Sozinha**, quando a sincronização traz um evento **novo** que já terminou;
- pelo menu: ação **Agenda**. Ela mostra `Agenda(3)` quando há pendentes, ou "Em dia!" quando não há;
- na tela do gato, uma **bolinha vermelha com número** avisa quantos estão esperando.

### A tela

```
┌────────────────────────────┐
│         Concluiu?          │
│          1 de 3            │
│ ┌────────────────────────┐ │
│ │   Reuniao de projeto   │ │  ← título (até 3 linhas, sem acentos)
│ └────────────────────────┘ │
│      terminou 14:30        │
│ [      BOOT = sim       ]  │  verde
│ [      PLUS = nao       ]  │  vermelho
│        PWR = depois        │
└────────────────────────────┘
```

| Botão | Efeito |
|---|---|
| **BOOT** (sim) | grava `✅` no título do evento → **energia +15**, humor recalculado, pose "Boa!" |
| **PLUS** (não) | grava `❌` no título → **energia −15**, humor recalculado, pose "Tudo bem" |
| **PWR** (depois) | volta ao gato; o evento continua pendente (até 24 h, depois conta como "não") |

Depois de responder, se houver outro pendente, a tela volta para ele.

### Primeiro o Google, depois o gato

A energia **só muda depois que o Google confirma** que gravou o ✅/❌. Se não houver rede, aparece
"Sem rede" e o evento continua pendente. Assim, o gato e a agenda nunca ficam diferentes.

### Títulos sem acentos

A fonte da tela só tem letras sem acento (ASCII). A função `asciiSimples()` converte o texto,
que chega da internet em **UTF-8**:

| Original | Na tela |
|---|---|
| `Reunião de orçamento` | `Reuniao de orcamento` |
| `✅ Almoço com João` | `Almoco com Joao` |
| `🎉 Festa` | `Festa` |

No UTF-8, letras acentuadas ocupam **2 bytes** (`ã` = `C3 A3`) e emojis ocupam **4**. A função
troca os acentuados pela letra simples e pula o resto, avançando byte a byte para nunca passar do
fim do texto, mesmo se o título vier cortado no meio de um emoji.

---

### Passo 5 (complemento) — Conferir qual versão da ponte está no ar

Salvar o `Codigo.gs` no editor **não** atualiza o que a URL `/exec` responde: o app da web roda
uma **versão congelada** do código, escolhida na implantação. Para não precisar adivinhar, toda
resposta da ponte traz o campo `"versao"`:

```js
const VERSAO = '2026-10-02.1';   // no topo do Codigo.gs — mude a cada alteração
```

**Conferir pelo terminal:**
```bash
apps-script/testar.sh
# ✅ versão implantada = local (2026-10-02.1)
# ou
# ❌ ainda está a versão antiga (sem campo versao) — local é 2026-10-02.1
```

**Conferir pelo navegador:** abra a URL `/exec`. O GET não devolve dados da agenda, só
`{"ok":false,"erro":"use POST","versao":"…"}`.

**Reimplantar sem mudar a URL:**
1. Cole o `Codigo.gs` inteiro no editor e salve (Ctrl+S). Arquivo com alteração não salva mostra um ponto no nome.
2. **Implantar → Gerenciar implantações** → selecione a implantação **ativa** → **✏️** →
   **Versão: Nova versão** → **Implantar**.

| Armadilha | O que acontece |
|---|---|
| Salvou, mas não reimplantou | a URL continua com o código antigo |
| Usou **Nova implantação** | cria **outra URL**; o `segredos.h` aponta para a antiga |
| Usou **Testar implantações** (`/dev`) | essa URL só funciona logada; não é a do aparelho |
| Esqueceu de mudar `VERSAO` | o teste não consegue distinguir as versões |

---

## Testes unitários — testar a lógica sem a placa

### Por que

Até aqui, cada mudança era testada **gravando na placa** e olhando a tela. Isso é lento e não
pega tudo: um erro de memória pode "funcionar" hoje e travar o gato daqui a uma semana.

Um **teste unitário** é um pequeno programa que chama **uma função** com entradas conhecidas e
confere se a saída é a esperada. Ele roda no **PC**, em segundos, quantas vezes quisermos.

### A regra: separar lógica de hardware

O PC não tem tela ST7789, botões nem Wi-Fi da placa. Então só dá para testar no PC o código que
**não depende de hardware**:

```
firmware/lembregotchi/
├── src/logica/        ← LÓGICA PURA: só C++ padrão → testada no PC (make test)
│                        (hoje em firmware/libraries/Lembregotchi/src/logica/ — ver abaixo)
│   └── texto.cpp          asciiSimples()
├── pet.cpp            ← usa tela, millis(), Preferences → testado na placa
└── ...
test/unit/
├── main.cpp           ← "liga" o framework de testes
└── test_texto.cpp     ← testes do texto.cpp
```

O `pet.cpp` continua desenhando e lendo botões, mas **chama** as funções de `src/logica/`. Na
Arduino IDE, a pasta `src/` é compilada junto com o sketch, então a mesma lógica roda no PC e na placa.

### O framework: doctest

O [doctest](https://github.com/doctest/doctest) é uma biblioteca de testes para C++ num único
arquivo (`test/vendor/doctest.h`, licença MIT). Um teste fica assim:

```cpp
TEST_CASE("asciiSimples: acentos do portugues viram letra simples") {
  CHECK(converter("Reunião de orçamento") == "Reuniao de orcamento");
}
```

- `TEST_CASE("nome")`: um grupo de verificações sobre um comportamento;
- `CHECK(condição)`: se for falsa, o teste **falha** e mostra os valores;
- `SUBCASE("nome")`: variações dentro do mesmo caso.

### Rodar

```bash
make test
```
```
[doctest] test cases:  6 |  6 passed | 0 failed | 0 skipped
[doctest] assertions: 17 | 17 passed | 0 failed |
[doctest] Status: SUCCESS!
```

O `Makefile` compila com:
- `-Wall -Wextra -Werror`: qualquer aviso do compilador vira erro;
- **AddressSanitizer** (`-fsanitize=address`): para o programa se houver leitura ou escrita fora
  de um buffer;
- **UBSan** (`-fsanitize=undefined`): para em comportamento indefinido (ex.: estouro de inteiro).

### O primeiro teste já achou um bug

Ao escrever os casos de borda do `asciiSimples()`, apareceu este: com um buffer de **tamanho 0**,
a função escrevia o terminador `\0` **fora** do buffer. O teste:

```cpp
SUBCASE("buffer de 0 bytes: não escreve nada") {
  char saida[1] = { 'X' };
  asciiSimples("abc", saida, 0);
  CHECK(saida[0] == 'X');   // nada pode ter sido escrito
}
```

A correção foi uma linha: `if (n == 0) return;`. Para provar que o teste protege mesmo,
removemos a correção numa cópia e rodamos de novo: o teste **falhou**, como devia. Essa técnica
se chama **teste de mutação**: estragar o código de propósito para ver se o teste percebe.

### O teste também pode estar errado

Na primeira execução, um caso falhou: esperávamos que `"æ ø ÷"` virasse `"  "` (dois espaços), mas
a função devolveu `""`. O código estava certo: ela **tira espaços do começo**. O erro era a
expectativa. Quando um teste falha, investigue os dois lados.

### Como adicionar um teste novo

1. A função vai para `src/logica/<modulo>.cpp` (+ `.h`), **sem** `Arduino.h`, `millis()` ou `gfx`.
   Valores como "hora atual" entram como **parâmetro**.
2. Crie `test/unit/test_<modulo>.cpp` com `#include "doctest.h"` e os `TEST_CASE`.
3. `make test` (o `Makefile` encontra os arquivos novos sozinho).
4. Compile também para a placa (`arduino-cli compile`), porque a lógica roda nos dois.

| Problema | Causa provável |
|---|---|
| `doctest.h: No such file` | rodou o `g++` à mão sem `-Itest/vendor`; use `make test` |
| `undefined reference to ...` | o `.cpp` da lógica não está em `src/logica/` |
| `AddressSanitizer: ...` | acesso fora de memória, que é um bug real; leia a linha indicada |
| Arduino não acha o `.h` | use `#include <Lembregotchi.h>` e compile com `make compilar` |

---

## Organização: uma biblioteca para tudo

### O problema

Os pinos da placa estavam escritos em **três lugares**: no `config.h` do firmware, no teste da tela
e no teste dos botões. Se um pino mudasse, era preciso lembrar de trocar nos três, e um teste podia
"passar" usando valores diferentes dos do firmware.

### A solução: uma biblioteca Arduino do próprio projeto

```
firmware/libraries/Lembregotchi/
├── library.properties        ← "carteira de identidade" da biblioteca (nome, dependências)
└── src/
    ├── Lembregotchi.h        ← o único include que os sketches precisam
    ├── placa/placa.h/.cpp    ← pinos + placaIniciar(), placaNovaTela(), placaLuz()
    └── logica/texto.h/.cpp   ← lógica pura (testada no PC)
```

Quem usa:

| Quem | Como inclui | Onde |
|---|---|---|
| Firmware | `#include <Lembregotchi.h>` | `firmware/lembregotchi/` |
| Testes de placa | `#include <Lembregotchi.h>` | `test/placa/passo1_tela/`, `test/placa/passo2_botoes/` |
| Testes unitários | `#include "logica/texto.h"` | `test/unit/` (doctest) |

Assim, **o teste mínimo usa exatamente o mesmo código** que o firmware. Exemplo do teste da tela:

```cpp
#include <Lembregotchi.h>

Arduino_GFX *gfx = placaNovaTela();   // mesma tela, mesmos pinos do firmware

void setup() {
  gfx->begin();
  placaLuz(true);
  ...
```

### Dois tipos de teste

| Tipo | Pasta | Roda onde | Como confere |
|---|---|---|---|
| **Unitário** | `test/unit/` | no PC | sozinho: `make test` diz passou/falhou |
| **De placa** | `test/placa/` | na placa | você olha a tela / aperta os botões |

### Comandos (`Makefile`)

```bash
make test                       # testes unitários no PC
make compilar                   # compila o firmware e todos os testes de placa
make gravar                     # grava o firmware
make gravar-teste T=passo1_tela # grava um teste de placa
make ide                        # deixa a biblioteca visível na Arduino IDE
```

> A Arduino IDE só procura bibliotecas em `~/Arduino/libraries`. O `make ide` cria lá um **link**
> para a pasta do projeto: a IDE enxerga a biblioteca, mas o código continua num lugar só.

---

## Passo 8 — Tags: categorias de eventos com #hashtag

### Como usar

No Google Calendar (celular ou PC), coloque uma **hashtag no título** do evento:

```
Estudar React #estudo
Corrida #saude
Reunião de time #trabalho
```

- só a **primeira** hashtag conta, em minúsculas (`#Estudo` = `#estudo`);
- `C#` ou `#` sozinho **não** são tags (precisa de espaço antes e letras depois).

### No aparelho

| Onde | O que aparece |
|---|---|
| "Concluiu?" | o título **sem** a hashtag, e embaixo `14:30  #estudo` |
| Nova ação **Tags** | as 5 tags mais usadas na semana, com `concluídos/total` e uma barra verde/vermelha |

### Quem faz o quê

| Parte | Onde | Testado em |
|---|---|---|
| Achar a tag e somar ✅/❌ por tag | ponte `Codigo.gs` (`extrairTag_`, `tagsMaisUsadas_`) | `test/apps-script/` (node:test) |
| Tirar a hashtag do título para mostrar | biblioteca `logica/tags` (`removerTags`) | `test/unit/test_tags.cpp` |

> A ponte mudou (versão `2026-10-03.1`): **reimplante** o `Codigo.gs` e confira com
> `apps-script/testar.sh`. Com a ponte antiga, a tela Tags mostra "Nenhuma tag".

---

## Passo 9 — Pomodoro ("Foco")

### O fluxo

```
[Foco] no menu do gato
   │ BOOT
   ▼
Pomodoro (gato)  ──BOOT──►  Selecionar Tempo ──► 5, 10 … 60 min ─┐
                            Foco Baixo      (25 min + 5 de pausa) ├─► timer ──► gato feliz +N
                            Foco Guerreiro  (50 min + 10 de pausa)┘              │
                                                                     (se houver pausa) ▼
                                                                    pausa (gato dormindo) ──► "Bora!"
```

| Tela | PLUS | BOOT | PWR |
|---|---|---|---|
| Pomodoro (gato) | menu | menu | volta ao gato |
| Menu / Selecionar Tempo | próximo item | escolhe / inicia | volta uma tela |
| Timer | — | pausa / continua | **desiste** (sem recompensa) |

### Recompensa

Ao **completar** o foco: **+1 de humor e +1 de energia por minuto** (máximo +50).
`Foco Baixo` = +25, `Foco Guerreiro` = +50. Desistir não dá nada.

Com a agenda ligada, o humor é recalculado pela semana a cada 5 minutos, e a recompensa sumiria.
Por isso ela vai para um **bônus de humor** guardado à parte: soma por cima da taxa da semana e
cai 2 pontos por hora, como o humor normal.

### O timer sem relógio: `millis()`

O timer guarda **quando a fase começou** (`inicioMs`) e calcula `restante = duração − (agora − início)`.
A conta usa números **sem sinal** de 32 bits, então continua certa mesmo quando o `millis()`
"dá a volta" (volta a zero depois de ~49 dias ligado). Isso está testado.

Pausar guarda `parouEmMs`; ao continuar, o início é empurrado para frente pelo tempo parado.

### A imagem do timer (troque pela sua!)

A imagem padrão é o cavaleiro descansando na fogueira (`image/descansa-guerreiro.jpeg`), uma
brincadeira compartilhada publicamente pela FromSoftware, escolhida pela dona do projeto.
**Quem usar este repositório pode colocar a imagem que quiser:**

```bash
make imagem IMG=minha-foto.jpg                     # recorta o centro na proporção certa
make imagem IMG=minha-foto.jpg CORTE="0 0 400 270" # ou escolha a área: x0 y0 x1 y1 (em pixels)
make imagem                                        # volta para a imagem padrão
make gravar
```

Por baixo, o `make imagem` chama o `tools/imagem_rgb565.py`. A tela só entende pixels **RGB565**
(2 bytes por pixel), então o script:
1. recorta a área escolhida (ou o centro, sem esticar);
2. redimensiona para **220×150**;
3. gera `src/imagens/foco.h` com os pixels (`img_foco`, **66 KB** na flash).

O `foco.cpp` desenha com `gfx->draw16bitRGBBitmap()`. Como o nome é sempre `img_foco`, trocar a
imagem **não exige mudar código**.

### Testes

| O quê | Arquivo |
|---|---|
| fases, pausa, volta do `millis()`, recompensa, `MM:SS` | `test/unit/test_pomodoro.cpp` |
| conversor de imagem: cores, recorte central, recorte manual | `test/tools/test_imagem_rgb565.py` |
| humor da semana + bônus, limites 0–100 | `test/unit/test_humor.cpp` |

| Problema | Causa provável |
|---|---|
| Timer não anda | está **pausado** (aparece "PAUSADO") |
| Não ganhou recompensa | apertou PWR (desistiu) antes do fim |
| Imagem com cores estranhas | `.h` gerado à mão ou de outro tamanho; gere de novo com `make imagem` |
| Imagem cortada no lugar errado | use `CORTE="x0 y0 x1 y1"` para escolher a área |
