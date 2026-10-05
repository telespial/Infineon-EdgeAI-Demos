#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-}"
if [[ -z "$BUILD_DIR" || ! -d "$BUILD_DIR" ]]; then
  echo "Usage: $0 <build_dir>" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="$ROOT_DIR/binaries"
MANIFEST="$BIN_DIR/manifest.json"
mkdir -p "$BIN_DIR"

mapfile -t ARTIFACTS < <(find "$BUILD_DIR" -maxdepth 4 -type f \( -name '*.elf' -o -name '*.hex' -o -name '*.bin' \) | sort)

if [[ ${#ARTIFACTS[@]} -eq 0 ]]; then
  echo "No .elf/.hex/.bin artifacts found under: $BUILD_DIR" >&2
  exit 1
fi

for f in "${ARTIFACTS[@]}"; do
  cp -f "$f" "$BIN_DIR/"
done

python3 - "$BIN_DIR" "$MANIFEST" << 'PY'
import hashlib, json, sys, datetime
from pathlib import Path
bin_dir = Path(sys.argv[1])
man = Path(sys.argv[2])
arts = []
for p in sorted(bin_dir.iterdir()):
    if p.suffix.lower() not in {'.elf', '.hex', '.bin'}:
        continue
    h = hashlib.sha256(p.read_bytes()).hexdigest()
    arts.append({"file": p.name, "size": p.stat().st_size, "sha256": h})
obj = {"board": "KIT_PSE84_AI", "last_updated_utc": datetime.datetime.utcnow().isoformat() + "Z", "artifacts": arts}
man.write_text(json.dumps(obj, indent=2) + "\n", encoding="utf-8")
PY

echo "Published ${#ARTIFACTS[@]} artifact(s) to: $BIN_DIR"
