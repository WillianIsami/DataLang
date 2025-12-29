#!/usr/bin/env bash

set -e

if [ -z "$1" ]; then
  echo "Uso: $0 <ast.json>" >&2
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" >/dev/null 2>&1 && pwd)"
JSON_FILE="$1"
VERIFIER="$SCRIPT_DIR/build/exec/datalang_verify"
SCHEME_FALLBACK="$SCRIPT_DIR/build/exec/datalang_verify_app/datalang_verify.ss"

# Tenta usar Idris2 se disponível; caso contrário, cai no script Chez.
if command -v idris2 >/dev/null 2>&1; then
  if [ ! -x "$VERIFIER" ] || [ "$SCRIPT_DIR/Main.idr" -nt "$VERIFIER" ]; then
    echo "Compilando verificador Idris..." >&2
    (cd "$SCRIPT_DIR" && idris2 --build datalang_verify.ipkg) || {
      echo "Falha ao compilar verificador Idris." >&2
      exit 1
    }
  fi
  exec "$VERIFIER" "$JSON_FILE"
elif [ -x "$SCHEME_FALLBACK" ]; then
  exec "$SCHEME_FALLBACK" "$JSON_FILE"
else
  echo "Nem idris2 nem fallback do verificador estão disponíveis." >&2
  exit 1
fi
