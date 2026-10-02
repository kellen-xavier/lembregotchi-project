
---

## Passo 3 — O gato na tela (lógica do nekogotchi)

Código: `firmware/lembregotchi/` (abra o `lembregotchi.ino` na Arduino IDE; os outros arquivos
abrem juntos como abas).

### Organização dos arquivos

| Arquivo | O que faz |
|---|---|
| `lembregotchi.ino` | liga a tela e os botões, e chama o bichinho no `loop()` |
| `config.h` | pinos da placa e números do bichinho (quanto a fome cai por hora etc.) |
| `pet.h` / `pet.cpp` | o bichinho: estado, passagem do tempo, humor, desenho e ações |
| `src/cat_sprites/` | os 8 desenhos do gato (do nekogotchi, licença MIT) |

> Na Arduino IDE, só a pasta `src/` é compilada junto com o sketch. Por isso os desenhos ficam ali.

### Como funciona

**1. Três números de 0 a 100**

| Valor | Sobe com | Cai com |
|---|---|---|
| **Comida** | Comer (+50) | o tempo (−3 por hora) |
| **Humor** | Brincar (+20), Carinho (+10) | o tempo (−2 por hora) |
| **Energia** | o tempo / descanso (+8 por hora) | Brincar (−15) |

**2. Humor → desenho e cor de fundo** (o primeiro que valer, de cima para baixo)

| Condição | Desenho | Fundo |
|---|---|---|
| energia < 20 | dormindo | azul-escuro |
| humor < 15 **e** comida < 15 | triste | azul-acinzentado |
| comida < 30 | com fome | laranja-claro |
| comida > 70 **e** humor > 70 | feliz | verde-claro |
| senão | contente | creme |

**3. Os desenhos**

Cada desenho tem 150×150 pixels de **1 bit** (cada pixel é "pinta" ou "não pinta"). Cada linha
ocupa 19 bytes (150 bits arredondados para cima). É o mesmo formato que a função
`gfx->drawBitmap()` entende. Na tela colorida, escolhemos a **cor da tinta** e a **cor do fundo**,
por isso o mesmo desenho pode ficar de cores diferentes conforme o humor.

**4. Canvas: desenhar sem piscar**

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

**6. Salvar na memória (Preferences)**

O estado é salvo na flash interna (biblioteca `Preferences`, que já vem com o ESP32) a cada ação e
a cada 5 minutos. Ao religar, o gato volta como estava.

### Controles

| Botão | Ação |
|---|---|
| **PLUS** | escolhe a próxima ação: Comer → Brincar → Carinho → Status |
| **BOOT** | executa a ação escolhida (a pose aparece por ~1,4 s) |
| qualquer um, na tela Status | volta |

### Tela principal

```
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

```
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
|---|---|
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

```
ESP32 ──POST {chave, acao}──► Apps Script ──► sua agenda
      ◄──── JSON pequeno ────
```

Assim, o ESP32 **não faz login no Google** e **não guarda senha do Google**. Ele só conhece a URL e
uma chave.

Arquivos (públicos, **sem segredos**):

| Arquivo | O que é |
|---|---|
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
|---|---|
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

**2. Gerar a chave (no SEU terminal, não no chat)**

Este comando gera 64 caracteres aleatórios e grava direto no `segredos.h`:
```bash
cd ~/Projects/lembregotchi-project
CHAVE=$(openssl rand -hex 32)
sed -i "s|^#define LEMBREGOTCHI_CHAVE .*|#define LEMBREGOTCHI_CHAVE \"$CHAVE\"|" firmware/lembregotchi/segredos.h
echo "$CHAVE"   # copie para o próximo passo
unset CHAVE
```

**3. Guardar a chave no Apps Script**
1. Em **⚙️ Configurações do projeto → Propriedades do script → Adicionar propriedade**:
   - Propriedade: `LEMBREGOTCHI_CHAVE`
   - Valor: a chave copiada
2. Salve. No editor, escolha a função **`testarChaveConfigurada`** e clique em **Executar**.
   - Na primeira execução, o Google pede autorização: **Revisar permissões** → escolha sua conta →
     "O Google não verificou este app" → **Avançado** → **Acessar Lembregotchi (não seguro)** →
     **Permitir**. Esse aviso aparece porque o script é seu e não passou pela verificação do Google.
   - O log deve mostrar `Chave configurada (64 caracteres)`.

**4. Testar a leitura da agenda (pelo editor)**
- Execute **`testarResumo`**. O log mostra o JSON com os seus eventos do último dia.

**5. Publicar como app da web**

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

**6. Testar pelo terminal**
```bash
apps-script/testar.sh             # deve responder {"ok":true, ...}
apps-script/testar.sh sem-chave   # deve responder {"ok":false,"erro":"nao autorizado"}
```

---

## Passo 6 — O ESP32 busca a agenda

Código novo: `firmware/lembregotchi/calendario.cpp` e `certificados.h`. Biblioteca nova: **ArduinoJson 7.4.3**.

### O que acontece a cada 5 minutos

```
ESP32 ──POST {chave, "resumo", desde}──► Apps Script
      ◄── {criados, semana:{sim,nao}, pendentes[]} ──
```

| Dado recebido | Efeito no gato |
|---|---|
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
|---|---|
| `Agenda ok  2 pend.` (verde) | sincronizou nos últimos 30 min; 2 eventos aguardando check |
| `Agenda sem sinal` (vermelho) | configurada, mas não conseguiu falar com a ponte |
| `Agenda nao config.` | `LEMBREGOTCHI_URL` / `LEMBREGOTCHI_CHAVE` vazios no `segredos.h` |

### Problema comum: Wi-Fi não conecta

O ESP32 **só enxerga redes de 2,4 GHz**. Muitos roteadores têm duas redes (ex.: `Casa` e
`Casa_5G`), ou uma só com "band steering", que junta as duas.
- Use o nome da rede **2,4 GHz**, exatamente igual (maiúsculas e minúsculas contam).
- Se o roteador junta as bandas num nome só, ative uma rede 2,4 GHz separada nas configurações dele.
