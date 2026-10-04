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

// Carrega o Codigo.gs com uma agenda falsa
function carregar(eventos = [], propriedades = { LEMBREGOTCHI_CHAVE: CHAVE }) {
  const ctx = {
    console,
    CalendarApp: {
      getDefaultCalendar: () => ({
        // Como no Google: devolve eventos que se sobrepõem ao intervalo [inicio, fim)
        getEvents: (inicio, fim) => eventos.filter(
          (e) => e.getStartTime() < fim && e.getEndTime() > inicio),
      }),
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

// ─── Tags ────────────────────────────────────────────────────────────────────

test('extrairTag_: primeira hashtag, em minúsculas, sem o #', () => {
  const { extrairTag_ } = carregar();
  assert.equal(extrairTag_('Estudar React #Estudo'), 'estudo');
  assert.equal(extrairTag_('#trabalho Reunião'), 'trabalho');
  assert.equal(extrairTag_('Treino #saude #manha'), 'saude');
  assert.equal(extrairTag_('Médico #Saúde'), 'saúde');
  assert.equal(extrairTag_('✅ Revisão #estudo'), 'estudo');
});

test('extrairTag_: sem tag devolve vazio ("C#" e "#" sozinho não são tags)', () => {
  const { extrairTag_ } = carregar();
  assert.equal(extrairTag_('Reunião de projeto'), '');
  assert.equal(extrairTag_('Curso de C# básico'), '');
  assert.equal(extrairTag_('Sala # 3'), '');
  assert.equal(extrairTag_(''), '');
  assert.equal(extrairTag_(undefined), '');
});

test('extrairTag_: corta nomes muito longos', () => {
  const { extrairTag_ } = carregar();
  assert.equal(extrairTag_('#' + 'a'.repeat(50)).length, 20);
});

// ─── Resumo ──────────────────────────────────────────────────────────────────

test('resumo_: ignora eventos de antes do inicio', () => {
  const ctx = carregar([
    evento({ titulo: 'Antigo #estudo', fim: AGORA - 3 * D }),
    evento({ titulo: '❌ Antigo marcado', fim: AGORA - 2 * D }),
  ]);
  const r = ctx.resumo_(0, s(AGORA - D), AGORA);
  assert.deepEqual({ ...r.semana }, { sim: 0, nao: 0 });
  assert.equal(r.pendentes.length, 0);
  assert.equal(r.tags.length, 0);
});

test('resumo_: conta ✅, ❌ e esquecidos (> 24 h) por tag', () => {
  const ctx = carregar([
    evento({ titulo: '✅ Ler #estudo', fim: AGORA - 2 * H }),
    evento({ titulo: '✅ Aula #estudo', fim: AGORA - 3 * D }),
    evento({ titulo: '❌ Corrida #saude', fim: AGORA - 4 * H }),
    evento({ titulo: 'Esquecido #saude', fim: AGORA - 2 * D }),   // sem resposta há > 24 h
    evento({ titulo: '✅ Sem tag', fim: AGORA - 5 * H }),
  ]);
  const r = ctx.resumo_(0, s(AGORA - 6 * D), AGORA);
  assert.deepEqual({ ...r.semana }, { sim: 3, nao: 2 });
  assert.deepEqual(JSON.parse(JSON.stringify(r.tags)), [
    { tag: 'estudo', sim: 2, nao: 0 },
    { tag: 'saude', sim: 0, nao: 2 },
  ]);
});

test('resumo_: pendentes são os sem resposta das últimas 24 h, com a tag', () => {
  const ctx = carregar([
    evento({ titulo: 'Reunião #trabalho', fim: AGORA - 2 * H, id: 'a' }),
    evento({ titulo: 'Ainda acontecendo', fim: AGORA + H }),          // não terminou
    evento({ titulo: 'Feriado', fim: AGORA - 3 * H, diaInteiro: true }),
  ]);
  const r = ctx.resumo_(0, s(AGORA - 6 * D), AGORA);
  assert.equal(r.pendentes.length, 1);
  assert.deepEqual({ ...r.pendentes[0] },
    { id: 'a', titulo: 'Reunião #trabalho', tag: 'trabalho', fim: s(AGORA - 2 * H) });
  assert.deepEqual({ ...r.semana }, { sim: 0, nao: 0 });
});

test('resumo_: no máximo 5 tags, as mais usadas primeiro', () => {
  const eventos = [];
  ['a', 'b', 'c', 'd', 'e', 'f'].forEach((tag, i) => {
    for (let n = 0; n <= i; n++) eventos.push(evento({ titulo: `✅ x${n} #${tag}`, fim: AGORA - (n + 1) * H }));
  });
  const r = carregar(eventos).resumo_(0, s(AGORA - 6 * D), AGORA);
  assert.deepEqual(Array.from(r.tags, (t) => t.tag), ['f', 'e', 'd', 'c', 'b']);
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
