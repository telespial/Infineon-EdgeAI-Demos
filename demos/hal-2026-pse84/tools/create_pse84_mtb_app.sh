#!/usr/bin/env bash
set -euo pipefail

# Scaffolds a ModusToolbox app project for KIT_PSE84_AI (CLI-only path).
# Requires project-creator-cli in PATH.

APP_DIR="${1:-$(pwd)/../workspace/pse84_blinky_app}"
TEMPLATE="mtb-example-psoc6-empty-app"
BOARD="KIT_PSE84_AI"

if ! command -v project-creator-cli >/dev/null 2>&1; then
  echo "project-creator-cli not found in PATH." >&2
  echo "Install full ModusToolbox tools package or source its environment setup script." >&2
  exit 1
fi

mkdir -p "$(dirname "$APP_DIR")"
project-creator-cli \
  --board-id "$BOARD" \
  --app-id "$TEMPLATE" \
  --user-app-name "$(basename "$APP_DIR")" \
  --target-dir "$(dirname "$APP_DIR")"

echo "Created app workspace at: $APP_DIR"
