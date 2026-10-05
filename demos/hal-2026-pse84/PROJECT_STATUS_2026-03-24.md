# EDGEAI HAL 2026 Infineon KIT PSE84 AI Demo - Status (2026-03-24)

## Current Build/Flash State
- Board link verified over KitProg3 CMSIS-DAP (`04b4:f155`).
- UART present at `/dev/ttyACM0` and serial output active at 115200 baud.
- Project builds and flashes from ModusToolbox CLI in current workflow.
- Runtime gesture output currently active, but classification stability is still under tuning.

## Gesture Behavior (Observed)
- Swipe up/down detection is partially working.
- Tap count occasionally over/under-classifies:
  - 1 tap can report as 2.
  - 2 taps can report as 3.
  - Some swipe motions are still misclassified as taps.

## Known Technical Issues
1. Tap/Swipe Cross-Talk
- Swipe events can trigger tap logic due to overlapping energy/derivative thresholds.
- Need stronger arbitration and lockout between swipe and tap states.

2. Event Debounce and Grouping
- Tap grouping timeout and refractory windows are sensitive to hand speed.
- Fast gestures may collapse into wrong count buckets.

3. Serial Signal-to-Noise
- Raw debug streams are noisy and not useful for user-level validation.
- Output should remain event-only:
  - `swipe up`
  - `swipe down`
  - `taps=1|2|3`

4. LED UX Mapping
- LED indications exist but need final consistency mapping to confirmed gesture events only.
- Remove ambiguous intermediate LED states to avoid confusion during tuning.

## Code/Architecture Notes
- Gesture logic currently mixes threshold heuristics with state-machine guards.
- Existing trained-model artifacts are present in workspace (`proj_cm55/model/*`) and should be integrated as the primary classifier path to reduce heuristic drift.
- Short-term: hard tap suppression during swipe evidence.
- Medium-term: migrate to model-first inference with confidence-gated event emission.

## Recommended Next Steps
1. Make tap/swipe mutually exclusive in runtime state machine.
2. Emit events only after finalized decision windows.
3. Keep serial output event-only (no raw stream by default).
4. Add scripted regression gestures:
   - 10x swipe up
   - 10x swipe down
   - 10x 1-tap
   - 10x 2-tap
   - 10x 3-tap
5. Track confusion matrix in docs per firmware revision.

## Open Risks
- Gesture variance across users and hand distances.
- Ambient motion/reflections may affect thresholds.
- Without model-first path, per-environment retuning may remain necessary.

