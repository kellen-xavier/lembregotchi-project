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

printf '{"chave":"%s","acao":"resumo","desde":%d}' "$CHAVE" "$DESDE" \
  | curl -sS -L --max-time 30 -H 'Content-Type: application/json' --data-binary @- "$URL"
echo
