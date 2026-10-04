/**
 * Lembregotchi — ponte entre o ESP32 e o Google Calendar.
 *
 * Este arquivo é PÚBLICO (está no repositório). Ele NÃO contém segredos:
 * a chave fica em Configurações do projeto → Propriedades do script → LEMBREGOTCHI_CHAVE.
 *
 * O aparelho faz POST com um JSON:
 *   { "chave": "...", "acao": "resumo", "desde": <epoch em segundos>, "inicio": <epoch> }
 *   { "chave": "...", "acao": "check", "id": "...", "fim": <epoch>, "feito": true|false }
 */

// Versão deste código. Volta em toda resposta ("versao") para conferir o que está implantado:
// mude a cada alteração (data.número) e confira com apps-script/testar.sh.
const VERSAO = '2026-10-03.1';

const MARCA_SIM = '✅';
const MARCA_NAO = '❌';

const JANELA_PENDENTE_H   = 24;  // evento terminado há até 24 h sem resposta → pergunta no aparelho
const JANELA_SEMANA_DIAS  = 7;   // humor = taxa de conclusão desta janela
const JANELA_CRIADOS_DIAS = 60;  // procura eventos novos de 7 dias atrás até 60 dias à frente
const MAX_PENDENTES       = 5;
const MAX_TITULO          = 40;  // o aparelho só precisa de um título curto
const MAX_TAGS            = 5;   // tags da semana devolvidas (as mais usadas)
const MAX_TAG             = 20;  // tamanho máximo do nome de uma tag

// ─── Entrada HTTP ────────────────────────────────────────────────────────────

function doPost(e) {
  let pedido;
  try {
    pedido = JSON.parse(e.postData.contents);
  } catch (err) {
    return resposta_({ ok: false, erro: 'json invalido' });
  }

  if (!chaveValida_(pedido.chave)) {
    return resposta_({ ok: false, erro: 'nao autorizado' });
  }

  try {
    if (pedido.acao === 'resumo') return resposta_(resumo_(Number(pedido.desde) || 0, Number(pedido.inicio) || 0));
    if (pedido.acao === 'check')  return resposta_(check_(String(pedido.id || ''), Number(pedido.fim) || 0, pedido.feito === true));
    return resposta_({ ok: false, erro: 'acao desconhecida' });
  } catch (err) {
    console.error(err);
    return resposta_({ ok: false, erro: 'erro interno' });   // sem detalhes para fora
  }
}

// GET não expõe dados (só saem por POST com a chave); devolve apenas a versão,
// para conferir pelo navegador qual código está implantado.
function doGet() {
  return resposta_({ ok: false, erro: 'use POST' });
}

// ─── Segurança ───────────────────────────────────────────────────────────────

function chaveValida_(recebida) {
  const certa = PropertiesService.getScriptProperties().getProperty('LEMBREGOTCHI_CHAVE');
  if (!certa || certa.length < 32) return false;          // sem chave configurada, ninguém entra
  if (typeof recebida !== 'string' || recebida.length !== certa.length) return false;
  // Comparação em tempo constante: não revela quantos caracteres acertou
  let diferenca = 0;
  for (let i = 0; i < certa.length; i++) diferenca |= certa.charCodeAt(i) ^ recebida.charCodeAt(i);
  return diferenca === 0;
}

function resposta_(obj) {
  obj.versao = VERSAO;
  return ContentService.createTextOutput(JSON.stringify(obj)).setMimeType(ContentService.MimeType.JSON);
}

// ─── Regras ──────────────────────────────────────────────────────────────────

function agenda_() {
  return CalendarApp.getDefaultCalendar();   // só a agenda principal
}

function marcado_(titulo) {
  if (titulo.indexOf(MARCA_SIM) === 0) return 'sim';
  if (titulo.indexOf(MARCA_NAO) === 0) return 'nao';
  return null;
}

function epoch_(data) {
  return Math.floor(data.getTime() / 1000);
}

/**
 * Resumo para o aparelho:
 *  - criados:   eventos criados depois de "desde" (alimentam o gato)
 *  - pendentes: eventos que terminaram nas últimas 24 h e ainda não têm ✅/❌
 *  - semana:    quantos ✅ e quantos ❌ (ou esquecidos > 24 h) nos últimos 7 dias
 *  - tags:      ✅/❌ da semana por tag (#hashtag no título), as MAX_TAGS mais usadas
 *
 * "inicio" é quando o Lembregotchi começou a acompanhar a agenda: eventos que terminaram
 * antes disso são ignorados (não pedem check e não contam como esquecidos).
 * "agoraFixo" só é usado pelos testes (test/apps-script/), para congelar o relógio.
 */
function resumo_(desde, inicio, agoraFixo) {
  const inicioData = new Date((inicio || 0) * 1000);
  const agora = agoraFixo ? new Date(agoraFixo) : new Date();
  const H = 3600 * 1000, D = 24 * H;
  const cal = agenda_();

  // Eventos novos (Comida)
  let criados = 0;
  if (desde > 0) {
    const desdeData = new Date(desde * 1000);
    cal.getEvents(new Date(agora.getTime() - JANELA_SEMANA_DIAS * D), new Date(agora.getTime() + JANELA_CRIADOS_DIAS * D))
      .forEach(function (ev) { if (ev.getDateCreated() > desdeData) criados++; });
  }

  // Semana (Humor), tags e pendentes (Energia)
  let sim = 0, nao = 0;
  const pendentes = [];
  const porTag = {};
  cal.getEvents(new Date(agora.getTime() - JANELA_SEMANA_DIAS * D), agora).forEach(function (ev) {
    if (ev.isAllDayEvent()) return;            // eventos de dia inteiro não pedem check
    const fim = ev.getEndTime();
    if (fim > agora) return;                   // ainda não terminou
    if (fim < inicioData) return;              // de antes do Lembregotchi existir

    const titulo = ev.getTitle();
    const tag = extrairTag_(titulo);
    const m = marcado_(titulo);
    if (m === 'sim') { sim++; contarTag_(porTag, tag, 'sim'); return; }
    if (m === 'nao') { nao++; contarTag_(porTag, tag, 'nao'); return; }

    if (agora.getTime() - fim.getTime() <= JANELA_PENDENTE_H * H) {
      if (pendentes.length < MAX_PENDENTES) {
        pendentes.push({ id: ev.getId(), titulo: titulo.slice(0, MAX_TITULO), tag: tag, fim: epoch_(fim) });
      }
    } else {
      nao++;                                   // esquecido há mais de 24 h conta como não concluído
      contarTag_(porTag, tag, 'nao');
    }
  });

  return { ok: true, agora: epoch_(agora), criados: criados, pendentes: pendentes,
           semana: { sim: sim, nao: nao }, tags: tagsMaisUsadas_(porTag) };
}

// ─── Tags (#hashtag no título) ───────────────────────────────────────────────

// Primeira hashtag do título, em minúsculas e sem o '#'. "Estudar #React" → "react"; sem tag → ""
function extrairTag_(titulo) {
  const m = /(?:^|\s)#([\p{L}\p{N}_-]+)/u.exec(titulo || '');
  return m ? m[1].toLowerCase().slice(0, MAX_TAG) : '';
}

function contarTag_(porTag, tag, campo) {
  if (!tag) return;
  if (!porTag[tag]) porTag[tag] = { tag: tag, sim: 0, nao: 0 };
  porTag[tag][campo]++;
}

// As MAX_TAGS tags com mais eventos (empate: ordem alfabética, para a resposta ser estável)
function tagsMaisUsadas_(porTag) {
  return Object.keys(porTag).map(function (k) { return porTag[k]; })
    .sort(function (a, b) { return (b.sim + b.nao) - (a.sim + a.nao) || (a.tag < b.tag ? -1 : 1); })
    .slice(0, MAX_TAGS);
}

/**
 * Grava ✅ ou ❌ no início do título. Só aceita eventos que:
 *  - terminaram nos últimos 7 dias,
 *  - batem com o id E com o horário de fim (eventos repetidos têm o mesmo id),
 *  - ainda não foram marcados.
 */
function check_(id, fim, feito) {
  if (!id || !fim) return { ok: false, erro: 'faltam dados' };

  const agora = new Date();
  const fimData = new Date(fim * 1000);
  if (fimData > agora || agora.getTime() - fimData.getTime() > JANELA_SEMANA_DIAS * 24 * 3600 * 1000) {
    return { ok: false, erro: 'fora da janela' };
  }

  const candidatos = agenda_().getEvents(new Date(fimData.getTime() - 24 * 3600 * 1000), new Date(fimData.getTime() + 1000));
  const ev = candidatos.find(function (c) { return c.getId() === id && epoch_(c.getEndTime()) === fim; });
  if (!ev) return { ok: false, erro: 'evento nao encontrado' };
  if (marcado_(ev.getTitle())) return { ok: true, jaMarcado: true };

  ev.setTitle((feito ? MARCA_SIM : MARCA_NAO) + ' ' + ev.getTitle());
  return { ok: true };
}

// ─── Testes (rodar pelo editor: selecione a função e clique em Executar) ──────

function testarResumo() {
  const umDiaAtras = Math.floor(Date.now() / 1000) - 24 * 3600;
  console.log(JSON.stringify(resumo_(umDiaAtras, umDiaAtras), null, 2));
}

function testarChaveConfigurada() {
  const c = PropertiesService.getScriptProperties().getProperty('LEMBREGOTCHI_CHAVE');
  console.log(c && c.length >= 32 ? 'Chave configurada (' + c.length + ' caracteres)' : 'FALTA configurar LEMBREGOTCHI_CHAVE (mínimo 32 caracteres)');
}
