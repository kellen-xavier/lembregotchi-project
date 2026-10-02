
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
