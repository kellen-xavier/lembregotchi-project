# Integração com o Google Calendar — desenho

Decidido em 2026-10-01.

## Regras do bichinho

| Valor | Sobe | Cai | Prazo |
|---|---|---|---|
| **Comida** | cada **evento novo criado** na agenda (+20) | passagem do tempo (−3/h) — se você não planeja nada, ele fica com fome | contínuo |
| **Energia** | cada **check "sim"** no aparelho (+15) | cada **check "não"** (−15), ou evento que terminou e ficou sem check | **o seu dia** (imediato) |
| **Humor** | taxa de conclusão alta nos **últimos 7 dias** | taxa baixa → triste ("depressed") | **a sua semana** (média) |

Humor = `100 × concluídos / (concluídos + não concluídos)` dos últimos 7 dias. Sem eventos na semana,
o humor fica neutro (50).

## O que é "concluído": check no Lembregotchi

O Google Calendar não tem "concluir evento". Então:

1. Quando um evento **termina**, o Lembregotchi mostra: *"Concluiu: Reunião X?"*
2. **BOOT = sim**, **PLUS = não**.
3. A resposta é **gravada no próprio evento** do Calendar, como fonte da verdade:
   - sim → `✅` no início do título
   - não → `❌` no início do título
4. Evento que terminou há mais de 24 h sem resposta conta como **não concluído**.

Assim, o histórico fica visível também no celular, e o humor semanal é calculado a partir da agenda.

## Arquitetura: Apps Script como ponte

```
┌──────────────┐  HTTPS GET/POST + chave  ┌─────────────────────┐   CalendarApp   ┌─────────────────┐
│ Lembregotchi │ ───────────────────────► │ Google Apps Script  │ ──────────────► │ Google Calendar │
│  (ESP32-S3)  │ ◄─────────────────────── │ (na conta da dona)  │ ◄────────────── │                 │
└──────────────┘   JSON pequeno (resumo)  └─────────────────────┘                 └─────────────────┘
```

- O script roda **na conta Google da usuária**, com permissão só da agenda dela.
- O ESP32 **não faz login no Google**: só chama a URL do script com uma **chave secreta**
  (guardada em `segredos.h`, fora do controle de versão).
- O script devolve um **resumo pequeno** em JSON, não a agenda inteira (o ESP32 tem pouca memória).

### Chamadas previstas

| Chamada | O que faz | Resposta (exemplo) |
|---|---|---|
| `GET ?acao=resumo&desde=<epoch>` | eventos criados desde a última sincronização, eventos que terminaram e aguardam check, taxa da semana | `{"criados":2,"pendentes":[{"id":"…","titulo":"Reunião X","fim":1759350000}],"semana":{"sim":5,"nao":2}}` |
| `POST acao=check&id=<id>&feito=1` | grava ✅ ou ❌ no título do evento | `{"ok":true}` |

### Cuidados

- O Apps Script responde com **redirecionamento** (302) para `googleusercontent.com`: o ESP32 precisa
  **seguir redirecionamentos** (`HTTPClient::setFollowRedirects`).
- **Não contar duas vezes**: o aparelho guarda a hora da última sincronização e só pede o que veio depois.
- **Sem internet**: o gato continua só com a passagem do tempo; sincroniza quando a rede voltar.
- Títulos de eventos aparecem na tela do aparelho — considere isso se a agenda tiver algo sensível.

## Etapas

| Passo | Entrega |
|---|---|
| 4 | Wi-Fi + hora real (NTP); o tempo com a placa desligada passa a contar |
| 5 | Apps Script na conta Google, testado pelo navegador |
| 6 | ESP32 busca o resumo e aplica Comida / Energia / Humor |
| 7 | Tela de check ("Concluiu?") com BOOT/PLUS, gravando ✅/❌ no evento |
