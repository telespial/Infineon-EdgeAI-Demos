# TODO (Easy Steps First)

## Immediate (Today)
- [x] Flash known-good radar app and verify serial output.
- [x] Switch UART to event-only gesture output (`swipe up`, `swipe down`, `taps=1..3`).
- [ ] Tune swipe direction separation (`up/down`) on this specific board orientation.
- [ ] Validate stable tap grouping for `1/2/3` under fast and slow hand motion.
- [ ] Create new app folder: `workspace/pse84_communicator_app`.
- [ ] Add base state machine skeleton (`IDLE`, `WAKE`, `LISTEN`, `PROCESS`, `RESPOND`).
- [ ] Add centralized event enum + event queue module.
- [ ] Add startup self-test logs for radar, camera, audio, wifi, bluetooth.

## Easy Wins
- [x] Add radar swipe/tap event logger (event-only serial protocol).
- [ ] Map gestures to consistent LED pattern changes after direction retune is complete.
- [ ] Add beep/sound hooks for wake, success, error.
- [ ] Add command parser for 5 simple intents: time, status, wifi check, led color, help.

## Core Features
- [ ] Integrate speech input (push-to-talk first).
- [ ] Integrate speech output (TTS adapter).
- [ ] Add known-person lookup from local profile store.
- [ ] Add unknown-person prompt flow.
- [ ] Add wifi task bridge (single HTTP task endpoint).

## Stretch
- [ ] Passive wake-word mode.
- [ ] Bluetooth phone bridge.
- [ ] Background multi-step task planner.
- [ ] Telemetry dashboard + performance logs.

## Validation Checklist
- [ ] Cold boot to ready state < 5s.
- [ ] Gesture-to-action latency < 300ms.
- [ ] Swipe up/down separation > 90% on 50-gesture mixed test.
- [ ] Tap grouping accuracy > 95% on 1/2/3-tap sequences.
- [ ] Voice roundtrip latency < 2.5s (cloud mode).
- [ ] Identity match flow < 1s with enrolled user.
- [ ] Recovery from wifi drop without reboot.
