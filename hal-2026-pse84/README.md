# EDGEAI HAL 2026 Infineon KIT PSE84 AI Demo

CLI-first development workspace for the Infineon KIT_PSE84_AI dev kit.

This repository contains:
- `workspace/` - active MTB workspace and app projects
- `demo_software/` - upstream demo sources
- `examples/` - focused examples and notes (including RGB LED findings)
- `binaries/` - binary metadata and release staging
- `tools/` - helper scripts for project creation and publishing

## Vision
Build a **Star Trek-style communicator** prototype:
- Radar gesture control (swipe-style interactions)
- Voice feedback + assistant responses
- Face recognition for known users
- Unknown-user onboarding prompt
- Wi-Fi/Bluetooth task execution pipeline

## Quick Start (Easy First)
1. Connect board over USB debug port (`KitProg3 CMSIS-DAP`).
2. Build/flash baseline app in `workspace/pse84_rgb_alive_app`.
3. Open serial monitor and verify event-only gesture output.
4. Validate gesture events (`swipe up`, `swipe down`, `taps=1..3`).
5. Iterate gesture tuning constants in `proj_cm33_ns/main.c`.

See [TODO.md](TODO.md) for step-by-step execution and [docs/COMMUNICATOR_PLAN.md](docs/COMMUNICATOR_PLAN.md) for system design.
Gesture calibration status and next tuning protocol are tracked in [docs/GESTURE_TUNING_STATUS.md](docs/GESTURE_TUNING_STATUS.md).

## Current Hardware Note
RGB LED behavior on the tested board is documented in `examples/README.md` and `examples/pse84_rgb_alive/README.md`.
Observed output indicates non-ideal blue channel behavior in current pin mapping/tests.

## Current Firmware Status (2026-03-13)
- Active app: `workspace/pse84_rgb_alive_app/proj_cm33_ns/main.c`
- Radar init/build/flash path is validated from CLI with ModusToolbox tools.
- UART output is in event-only mode:
  - `swipe up`
  - `swipe down`
  - `taps=1`
  - `taps=2`
  - `taps=3`
- Gesture stack uses a lightweight embedded classifier plus heuristic fallback:
  - Swipe classification confidence gate
  - Tap fallback path for strong radial poke events
  - Swipe/tap lockout to reduce cross-triggering

## Known Gesture Limitation
- Swipe direction (`up` vs `down`) is still hardware/orientation sensitive and requires additional per-board tuning.
- Captured sessions showed intermittent `SWIPE_UP` dominance for both physical up/down motions.
- This is documented and tracked in [TODO.md](TODO.md) under immediate tuning tasks.

## Current Status
Detailed build/runtime status and known gesture issues are tracked in:
- `PROJECT_STATUS_2026-03-24.md`
