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

## Ajuste (2026-10-02): data de início

No primeiro teste, a semana veio com `0 sim / 47 não`: todos os eventos antigos da agenda, de antes do
Lembregotchi existir, contavam como "não concluídos", e o gato já começaria deprimido.

Correção: o aparelho envia `inicio`, a hora da **primeira sincronização** (guardada na flash), e a ponte
**ignora eventos que terminaram antes disso**: eles não pedem check e não contam como esquecidos.

## Ajuste (2026-10-02): versão nas respostas

Depois de publicar a correção de `inicio`, a ponte continuava respondendo como a versão antiga
(`semana.nao` alto). Sem um identificador, não dava para saber qual código estava implantado.

Correção: `const VERSAO` no `Codigo.gs`, devolvida como `"versao"` em **toda** resposta (inclusive
GET e erros, porque não é dado sensível). O `apps-script/testar.sh` compara com o arquivo local.
O firmware não usa esse campo.

## Ajuste (2026-10-03): tags por hashtag

Categoria de evento = **primeira `#hashtag` do título** (ex.: `Ler #estudo`). Escolhida porque funciona
em qualquer app do Google Calendar, sem configurar cores nem criar agendas.

- A ponte devolve `tag` em cada pendente e `tags: [{tag, sim, nao}]` com as 5 tags mais usadas da semana
  (mesmas regras do humor: ✅ = sim; ❌ ou esquecido > 24 h = não; só depois de `inicio`).
- O aparelho mostra a tag no "Concluiu?" e tem uma tela **Tags** com concluídos/total por tag.
- Versão da ponte: `2026-10-03.1`.

## Ajuste (2026-10-04): tags removidas

As tags por hashtag foram **retiradas** do sistema por enquanto, a pedido da dona: a ponte não devolve
mais `tag`/`tags` (versão `2026-10-04.1`), e o aparelho não tem mais a tela Tags. O código está no
commit `e778b15`.

## Ajuste (2026-10-04): eventos de hoje

A ponte devolve `hoje`: quantos eventos de hoje ainda não terminaram (`getEventsForDay`, fuso da agenda).
Usado na linha **EVENTOS** da Home. Versão da ponte: `2026-10-04.2`.

## Ajuste (2026-10-04): agenda de outra conta

Propriedade do script `AGENDA_ID` (opcional): a ponte lê essa agenda (`getCalendarById`) em vez da principal.
Permite usar a agenda de outra conta Google, compartilhada com "Fazer alterações nos eventos", sem mudar a
ponte nem o aparelho. Versão da ponte: `2026-10-04.3`.
