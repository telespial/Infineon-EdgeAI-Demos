# Gesture Tuning Status (KIT_PSE84_AI)

## Current Serial Contract
The active firmware prints only finalized gesture events:
- `swipe up`
- `swipe down`
- `taps=1`
- `taps=2`
- `taps=3`

No raw per-frame `DBG` stream is emitted in normal operation.

## Current Detection Stack
- Embedded lightweight classifier for `BACKGROUND`, `TAP`, `SWIPE_UP`, `SWIPE_DOWN`
- Heuristic fallback for strong tap impulses
- Swipe/tap lockout windows to reduce cross-triggering

## Known Issue
- Up/down separation is still inconsistent on the tested hardware orientation.
- Some runs show strong `SWIPE_UP` bias for both physical up and down motions.

## Recommended Next Calibration Procedure
1. Collect three labeled windows:
   - 20x swipe up
   - 20x swipe down
   - 20x tap groups (1/2/3 mixed)
2. Re-enable temporary debug fields only during calibration.
3. Tune confidence/confirm constants from captured distributions.
4. Return to event-only serial mode for operator use.

## Tracking
See [../TODO.md](../TODO.md) for open tuning tasks and acceptance criteria.
