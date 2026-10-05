/**
 * Lembregotchi — ponte entre o ESP32 e o Google Calendar.
 *
 * Este arquivo é PÚBLICO (está no repositório). Ele NÃO contém segredos:
 * a chave fica em Configurações do projeto → Propriedades do script → LEMBREGOTCHI_CHAVE.
 *
 * Qual agenda ler: propriedade AGENDA_ID (opcional) com o "ID da agenda" do Google Calendar
 * (pode ser de OUTRA conta, compartilhada com "Fazer alterações nos eventos").
 * Sem AGENDA_ID, usa a agenda principal da conta que implantou o script.
 *
 * O aparelho faz POST com um JSON:
 *   { "chave": "...", "acao": "resumo", "desde": <epoch em segundos>, "inicio": <epoch> }
 *   { "chave": "...", "acao": "check", "id": "...", "fim": <epoch>, "feito": true|false }
 */

// Versão deste código. Volta em toda resposta ("versao") para conferir o que está implantado:
// mude a cada alteração (data.número) e confira com apps-script/testar.sh.
const VERSAO = '2026-10-04.3';

const MARCA_SIM = '✅';
const MARCA_NAO = '❌';

const JANELA_PENDENTE_H   = 24;  // evento terminado há até 24 h sem resposta → pergunta no aparelho
const JANELA_SEMANA_DIAS  = 7;   // humor = taxa de conclusão desta janela
const JANELA_CRIADOS_DIAS = 60;  // procura eventos novos de 7 dias atrás até 60 dias à frente
const MAX_PENDENTES       = 5;
const MAX_TITULO          = 40;  // o aparelho só precisa de um título curto

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

// A agenda configurada em AGENDA_ID, ou a principal. null = ID sem acesso (não compartilhada / errado).
function agenda_() {
  const id = PropertiesService.getScriptProperties().getProperty('AGENDA_ID');
  if (!id || !id.trim()) return CalendarApp.getDefaultCalendar();
  return CalendarApp.getCalendarById(id.trim());
}

const SEM_AGENDA = { ok: false, erro: 'agenda nao encontrada' };

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
 *  - hoje:      eventos de hoje que ainda não terminaram (Home do aparelho)
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
  if (!cal) return SEM_AGENDA;

  // Eventos novos (Comida)
  let criados = 0;
  if (desde > 0) {
    const desdeData = new Date(desde * 1000);
    cal.getEvents(new Date(agora.getTime() - JANELA_SEMANA_DIAS * D), new Date(agora.getTime() + JANELA_CRIADOS_DIAS * D))
      .forEach(function (ev) { if (ev.getDateCreated() > desdeData) criados++; });
  }

  // Semana (Humor) e pendentes (Energia)
  let sim = 0, nao = 0;
  const pendentes = [];
  cal.getEvents(new Date(agora.getTime() - JANELA_SEMANA_DIAS * D), agora).forEach(function (ev) {
    if (ev.isAllDayEvent()) return;            // eventos de dia inteiro não pedem check
    const fim = ev.getEndTime();
    if (fim > agora) return;                   // ainda não terminou
    if (fim < inicioData) return;              // de antes do Lembregotchi existir

    const titulo = ev.getTitle();
    const m = marcado_(titulo);
    if (m === 'sim') { sim++; return; }
    if (m === 'nao') { nao++; return; }

    if (agora.getTime() - fim.getTime() <= JANELA_PENDENTE_H * H) {
      if (pendentes.length < MAX_PENDENTES) {
        pendentes.push({ id: ev.getId(), titulo: titulo.slice(0, MAX_TITULO), fim: epoch_(fim) });
      }
    } else {
      nao++;                                   // esquecido há mais de 24 h conta como não concluído
    }
  });

  // Eventos de hoje que ainda vão acontecer (getEventsForDay usa o fuso da agenda)
  const hoje = cal.getEventsForDay(agora).filter(function (ev) { return ev.getEndTime() > agora; }).length;

  return { ok: true, agora: epoch_(agora), criados: criados, pendentes: pendentes,
           semana: { sim: sim, nao: nao }, hoje: hoje };
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

  const cal = agenda_();
  if (!cal) return SEM_AGENDA;
  const candidatos = cal.getEvents(new Date(fimData.getTime() - 24 * 3600 * 1000), new Date(fimData.getTime() + 1000));
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

// Mostra qual agenda a ponte está lendo (rode pelo editor depois de mudar AGENDA_ID)
function testarAgenda() {
  const id = PropertiesService.getScriptProperties().getProperty('AGENDA_ID');
  const cal = agenda_();
  if (!cal) {
    console.log('AGENDA_ID não encontrada: confira o ID e se a agenda foi compartilhada com esta conta.');
    return;
  }
  console.log((id ? 'AGENDA_ID: ' : 'Agenda principal desta conta: ') + cal.getName() +
              (cal.isOwnedByMe() ? ' (sua)' : ' (compartilhada com você)'));
}

function testarChaveConfigurada() {
  const c = PropertiesService.getScriptProperties().getProperty('LEMBREGOTCHI_CHAVE');
  console.log(c && c.length >= 32 ? 'Chave configurada (' + c.length + ' caracteres)' : 'FALTA configurar LEMBREGOTCHI_CHAVE (mínimo 32 caracteres)');
}
