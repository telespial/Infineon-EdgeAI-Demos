# Communicator Plan (PSE84 AI)

## 1. Product Goal
Create a wearable/desk communicator experience with:
- Gesture input (radar swipe)
- Conversational voice output
- Identity-aware greetings
- Network-backed task execution

## 2. System Architecture
- Input
  - Radar sensor -> gesture classifier -> event queue
  - Optional button/push-to-talk fallback
- Perception
  - Face detection + embedding + local identity store
- Interaction
  - Wake/SFX layer
  - ASR -> intent/task -> TTS
- Connectivity
  - Wi-Fi for cloud task APIs
  - Bluetooth for local device relay
- Core Runtime
  - Event bus + state machine (`IDLE`, `WAKE`, `LISTEN`, `PROCESS`, `RESPOND`)

## 3. Incremental Delivery
### Phase A - Board Foundation
- Stable build/flash pipeline
- LED + serial diagnostics
- Boot-time health checks

Status:
- Completed for current radar demo app (`workspace/pse84_rgb_alive_app`).
- CLI build/flash flow verified repeatedly using ModusToolbox and KitProg3.
- Serial diagnostics now use event-only protocol for easier operator testing.

### Phase B - Gesture Control
- Radar raw data capture
- Swipe detector (left/right/up/down)
- Gesture-driven UI state transitions

Status:
- In progress.
- Tap grouping (`1/2/3`) logic is implemented with classifier + fallback heuristic.
- Swipe direction remains orientation-sensitive on current board and needs more calibration data.
- Current serialized events:
  - `swipe up`
  - `swipe down`
  - `taps=1|2|3`

### Phase C - Voice Assistant Core
- Push-to-talk capture path
- ASR integration (local or cloud)
- TTS response playback

### Phase D - Identity Features
- Face detect + recognition
- Enrolled user greeting
- Unknown-user name prompt + optional enrollment

### Phase E - Task Agent + Connectivity
- Wi-Fi command gateway
- Bluetooth relay mode
- Tool/task adapters (timers, reminders, web actions)

## 4. Repo Layout Guidance
- `workspace/pse84_communicator_app/`
  - `src/core/` state machine + event bus
  - `src/gesture/` radar drivers + classifier
  - `src/voice/` asr + tts adapters
  - `src/vision/` face pipeline
  - `src/net/` wifi/bt task adapters
  - `src/ui/` leds + tones + status events

## 5. Risks and Constraints
- Audio latency on cloud ASR/TTS
- Face recognition memory footprint
- Privacy and consent for identity features
- Power budget if moving to battery operation
- Radar gesture direction ambiguity due board/sensor orientation and environment.
- False positives/negatives if swipe/tap thresholds are not tuned per deployment setup.

## 6. Definition of Done (MVP)
- User swipes to open communicator mode
- Device listens and executes one spoken task
- Known user greeted by name
- Unknown user prompted for name
- Spoken response returned over speaker/headset
