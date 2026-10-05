// Testes da ponte apps-script/Codigo.gs, sem Google: o código roda numa "caixa de areia" (vm)
// com CalendarApp, PropertiesService e ContentService falsos.
//   node --test test/apps-script/      (ou: make test)
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';

const CODIGO = readFileSync(new URL('../../apps-script/Codigo.gs', import.meta.url), 'utf8');
const H = 3600 * 1000, D = 24 * H;
const AGORA = Date.UTC(2026, 9, 3, 15, 0, 0);   // relógio congelado: 03/10/2026 12:00 em Brasília
const CHAVE = 'k'.repeat(64);

// Evento falso com a mesma "cara" do CalendarEvent do Google
function evento({ titulo, fim, duracao = H, criado = AGORA - 30 * D, diaInteiro = false, id }) {
  const ev = {
    titulo,
    getId: () => id || 'id-' + titulo,
    getTitle: () => ev.titulo,
    setTitle: (t) => { ev.titulo = t; },
    getStartTime: () => new Date(fim - duracao),
    getEndTime: () => new Date(fim),
    getDateCreated: () => new Date(criado),
    isAllDayEvent: () => diaInteiro,
  };
  return ev;
}

// Agenda falsa com a mesma "cara" do Calendar do Google
function agendaFalsa(eventos) {
  return {
        // Como no Google: devolve eventos que se sobrepõem ao intervalo [inicio, fim)
        getEvents: (inicio, fim) => eventos.filter(
          (e) => e.getStartTime() < fim && e.getEndTime() > inicio),
        // Como no Google: eventos do dia (aqui no fuso de Brasília, UTC−3)
        getEventsForDay: (dia) => {
          const inicio = Math.floor((dia.getTime() - 3 * H) / D) * D + 3 * H;
          return eventos.filter((e) => e.getStartTime() < new Date(inicio + D) && e.getEndTime() > new Date(inicio));
        },
  };
}

// Carrega o Codigo.gs com agendas falsas: a principal e, opcionalmente, outras por ID
function carregar(eventos = [], propriedades = { LEMBREGOTCHI_CHAVE: CHAVE }, outrasAgendas = {}) {
  const ctx = {
    console,
    CalendarApp: {
      getDefaultCalendar: () => agendaFalsa(eventos),
      getCalendarById: (id) => (outrasAgendas[id] ? agendaFalsa(outrasAgendas[id]) : null),
    },
    PropertiesService: { getScriptProperties: () => ({ getProperty: (k) => propriedades[k] ?? null }) },
    ContentService: {
      MimeType: { JSON: 'json' },
      createTextOutput: (texto) => ({ setMimeType: () => ({ corpo: JSON.parse(texto) }) }),
    },
  };
  vm.createContext(ctx);
  vm.runInContext(CODIGO, ctx);
  return ctx;
}

const s = (ms) => Math.floor(ms / 1000);
const post = (ctx, pedido) => ctx.doPost({ postData: { contents: JSON.stringify(pedido) } }).corpo;

// ─── Resumo ──────────────────────────────────────────────────────────────────

test('resumo_: ignora eventos de antes do inicio', () => {
  const ctx = carregar([
    evento({ titulo: 'Antigo #estudo', fim: AGORA - 3 * D }),
    evento({ titulo: '❌ Antigo marcado', fim: AGORA - 2 * D }),
  ]);
  const r = ctx.resumo_(0, s(AGORA - D), AGORA);
  assert.deepEqual({ ...r.semana }, { sim: 0, nao: 0 });
  assert.equal(r.pendentes.length, 0);
});

test('resumo_: conta ✅, ❌ e esquecidos (> 24 h)', () => {
  const ctx = carregar([
    evento({ titulo: '✅ Ler #estudo', fim: AGORA - 2 * H }),
    evento({ titulo: '✅ Aula #estudo', fim: AGORA - 3 * D }),
    evento({ titulo: '❌ Corrida #saude', fim: AGORA - 4 * H }),
    evento({ titulo: 'Esquecido #saude', fim: AGORA - 2 * D }),   // sem resposta há > 24 h
    evento({ titulo: '✅ Sem tag', fim: AGORA - 5 * H }),
  ]);
  const r = ctx.resumo_(0, s(AGORA - 6 * D), AGORA);
  assert.deepEqual({ ...r.semana }, { sim: 3, nao: 2 });
});

test('resumo_: pendentes são os sem resposta das últimas 24 h', () => {
  const ctx = carregar([
    evento({ titulo: 'Reunião #trabalho', fim: AGORA - 2 * H, id: 'a' }),
    evento({ titulo: 'Ainda acontecendo', fim: AGORA + H }),          // não terminou
    evento({ titulo: 'Feriado', fim: AGORA - 3 * H, diaInteiro: true }),
  ]);
  const r = ctx.resumo_(0, s(AGORA - 6 * D), AGORA);
  assert.equal(r.pendentes.length, 1);
  assert.deepEqual({ ...r.pendentes[0] },
    { id: 'a', titulo: 'Reunião #trabalho', fim: s(AGORA - 2 * H) });
  assert.deepEqual({ ...r.semana }, { sim: 0, nao: 0 });
});

test('resumo_: hoje conta os eventos de hoje que ainda não terminaram', () => {
  // AGORA = 03/10 12:00 em Brasília
  const ctx = carregar([
    evento({ titulo: 'Manhã (já passou)', fim: AGORA - 2 * H }),
    evento({ titulo: 'Tarde', fim: AGORA + 3 * H }),
    evento({ titulo: 'Agora mesmo', fim: AGORA + 30 * 60 * 1000 }),
    evento({ titulo: 'Amanhã', fim: AGORA + D }),
    evento({ titulo: 'Ontem', fim: AGORA - D }),
  ]);
  assert.equal(ctx.resumo_(0, s(AGORA - D), AGORA).hoje, 2);
});

test('resumo_: criados conta só eventos criados depois de "desde"', () => {
  const ctx = carregar([
    evento({ titulo: 'Novo', fim: AGORA + 2 * D, criado: AGORA - H }),
    evento({ titulo: 'Velho', fim: AGORA + 2 * D, criado: AGORA - 10 * D }),
  ]);
  assert.equal(ctx.resumo_(s(AGORA - 2 * H), s(AGORA - D), AGORA).criados, 1);
  assert.equal(ctx.resumo_(0, s(AGORA - D), AGORA).criados, 0);   // primeira sincronização: não alimenta
});

// ─── Segurança e HTTP ────────────────────────────────────────────────────────

test('doPost: recusa chave errada, curta ou ausente', () => {
  const ctx = carregar();
  assert.equal(post(ctx, { chave: 'x'.repeat(64), acao: 'resumo' }).erro, 'nao autorizado');
  assert.equal(post(ctx, { chave: 'k', acao: 'resumo' }).erro, 'nao autorizado');
  assert.equal(post(ctx, { acao: 'resumo' }).erro, 'nao autorizado');
});

test('doPost: sem chave configurada (ou curta), ninguém entra', () => {
  assert.equal(post(carregar([], {}), { chave: CHAVE, acao: 'resumo' }).erro, 'nao autorizado');
  assert.equal(post(carregar([], { LEMBREGOTCHI_CHAVE: 'curta' }), { chave: 'curta', acao: 'resumo' }).erro, 'nao autorizado');
});

test('doPost: JSON inválido e ação desconhecida', () => {
  const ctx = carregar();
  assert.equal(ctx.doPost({ postData: { contents: '{nao é json' } }).corpo.erro, 'json invalido');
  assert.equal(post(ctx, { chave: CHAVE, acao: 'apagar' }).erro, 'acao desconhecida');
});

test('toda resposta traz a versão; GET não traz dados', () => {
  const ctx = carregar();
  const versao = /const VERSAO = '([^']+)'/.exec(CODIGO)[1];
  assert.deepEqual({ ...ctx.doGet().corpo }, { ok: false, erro: 'use POST', versao });
  assert.equal(post(ctx, { chave: 'errada', acao: 'resumo' }).versao, versao);
});

// ─── Check ───────────────────────────────────────────────────────────────────

test('check_: grava ✅ ou ❌ no início do título', () => {
  const fim = Date.now() - 2 * H;   // check_ usa o relógio real
  const sim = evento({ titulo: 'Ler #estudo', fim, id: 's' });
  const nao = evento({ titulo: 'Correr', fim, id: 'n' });
  const ctx = carregar([sim, nao]);
  assert.equal(ctx.check_('s', s(fim), true).ok, true);
  assert.equal(ctx.check_('n', s(fim), false).ok, true);
  assert.equal(sim.titulo, '✅ Ler #estudo');
  assert.equal(nao.titulo, '❌ Correr');
});

test('check_: não marca duas vezes, nem evento futuro, antigo ou com fim diferente', () => {
  const fim = Date.now() - 2 * H;
  const ev = evento({ titulo: '✅ Já feito', fim, id: 'x' });
  const futuro = evento({ titulo: 'Depois', fim: Date.now() + H, id: 'f' });
  const ctx = carregar([ev, futuro]);
  assert.equal(ctx.check_('x', s(fim), true).jaMarcado, true);
  assert.equal(ev.titulo, '✅ Já feito');
  assert.equal(ctx.check_('f', s(Date.now() + H), true).erro, 'fora da janela');
  assert.equal(ctx.check_('x', s(Date.now() - 8 * D), true).erro, 'fora da janela');
  assert.equal(ctx.check_('x', s(fim) - 60, true).erro, 'evento nao encontrado');
  assert.equal(ctx.check_('', s(fim), true).erro, 'faltam dados');
});

// ─── Qual agenda (AGENDA_ID) ─────────────────────────────────────────────────

test('agenda: sem AGENDA_ID usa a agenda principal', () => {
  const ctx = carregar([evento({ titulo: '✅ Principal', fim: AGORA - H })]);
  assert.equal(ctx.resumo_(0, s(AGORA - D), AGORA).semana.sim, 1);
});

test('agenda: com AGENDA_ID lê a agenda compartilhada, não a principal', () => {
  const ctx = carregar(
    [evento({ titulo: '✅ Principal', fim: AGORA - H })],
    { LEMBREGOTCHI_CHAVE: CHAVE, AGENDA_ID: '  outra@gmail.com ' },        // espaços sobrando são ignorados
    { 'outra@gmail.com': [evento({ titulo: '❌ Da outra conta', fim: AGORA - H }),
                          evento({ titulo: '❌ Outra 2', fim: AGORA - 2 * H })] });
  const r = ctx.resumo_(0, s(AGORA - D), AGORA);
  assert.deepEqual({ ...r.semana }, { sim: 0, nao: 2 });
});

test('agenda: AGENDA_ID sem acesso responde "agenda nao encontrada" (resumo e check)', () => {
  const ctx = carregar([], { LEMBREGOTCHI_CHAVE: CHAVE, AGENDA_ID: 'nao-compartilhada@gmail.com' });
  assert.equal(ctx.resumo_(0, s(AGORA - D), AGORA).erro, 'agenda nao encontrada');
  assert.equal(ctx.check_('x', s(Date.now() - H), true).erro, 'agenda nao encontrada');
  assert.equal(post(ctx, { chave: CHAVE, acao: 'resumo' }).erro, 'agenda nao encontrada');
});

test('agenda: check grava na agenda do AGENDA_ID', () => {
  const fim = Date.now() - 2 * H;
  const ev = evento({ titulo: 'Reunião', fim, id: 'r' });
  const ctx = carregar([], { LEMBREGOTCHI_CHAVE: CHAVE, AGENDA_ID: 'outra@gmail.com' }, { 'outra@gmail.com': [ev] });
  assert.equal(ctx.check_('r', s(fim), true).ok, true);
  assert.equal(ev.titulo, '✅ Reunião');
});
