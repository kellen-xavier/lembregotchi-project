#!/usr/bin/env bash
# Testa a ponte do Apps Script usando a URL e a chave do segredos.h.
# Nada secreto é impresso nem passado na linha de comando (a chave vai pela entrada padrão).
#   uso: apps-script/testar.sh            → pede o resumo
#        apps-script/testar.sh sem-chave  → confirma que chave errada é recusada
set -euo pipefail
cd "$(dirname "$0")/.."
SEG=firmware/lembregotchi/segredos.h

valor() { sed -nE "s/^#define[[:space:]]+$1[[:space:]]+\"([^\"]*)\".*/\1/p" "$SEG"; }
URL=$(valor LEMBREGOTCHI_URL)
CHAVE=$(valor LEMBREGOTCHI_CHAVE)

[[ -n "$URL" && -n "$CHAVE" ]] || { echo "Preencha LEMBREGOTCHI_URL e LEMBREGOTCHI_CHAVE em $SEG"; exit 1; }
[[ "$URL" == https://script.google.com/* ]] || { echo "A URL deve começar com https://script.google.com/"; exit 1; }

[[ "${1:-}" == "sem-chave" ]] && CHAVE="chave-errada-de-proposito-000000000000000000000000000000000000000"
DESDE=$(( $(date +%s) - 86400 ))

# inicio = 1 dia atrás: simula um Lembregotchi que começou ontem
RESPOSTA=$(printf '{"chave":"%s","acao":"resumo","desde":%d,"inicio":%d}' "$CHAVE" "$DESDE" "$DESDE" \
  | curl -sS -L --max-time 30 -H 'Content-Type: application/json' --data-binary @- "$URL")
unset CHAVE
echo "$RESPOSTA"

# Confere se o código implantado é o mesmo do arquivo local
LOCAL=$(sed -nE "s/^const VERSAO = '([^']*)'.*/\1/p" apps-script/Codigo.gs)
NO_AR=$(printf '%s' "$RESPOSTA" | sed -nE 's/.*"versao":"([^"]*)".*/\1/p')
echo
if [[ -n "$NO_AR" && "$NO_AR" == "$LOCAL" ]]; then
  echo "✅ versão implantada = local ($LOCAL)"
else
  echo "❌ ainda está a versão ${NO_AR:-antiga (sem campo versao)} — local é $LOCAL"
  echo "   Reimplante: Implantar → Gerenciar implantações → ✏️ → Versão: Nova versão → Implantar"
  exit 2
fi
