# MASTER Product Plan (PSE84 AI KIT)

## Objective
Build a pin-style, voice-first, screen-light AI companion on KIT_PSE84_AI.

This is the single source of truth for:
- Product direction
- Feature backlog
- Build phases
- Decisions on what is in/out

## Product Vision
A compact wearable-style assistant that is:
- Hands-free first
- Event-driven
- Fast to interact with
- Local-first for privacy and reliability

## Core Interaction Loop
1. Wake (gesture/button/voice trigger)
2. Listen (short command capture)
3. Process (local intent first, cloud fallback)
4. Respond (short audio + LED confirmation)
5. Return to idle

## Feature Backlog (Add/Remove Here)
Use Status values: `planned`, `in_progress`, `blocked`, `done`, `deferred`, `removed`.

| ID | Feature | Status | Priority | Notes |
|---|---|---|---|---|
| F-01 | Reliable radar gestures (`swipe up/down`, `taps=1/2/3`) | planned | P0 | Must stabilize arbitration and confusion matrix tracking |
| F-02 | Voice interaction loop (wake/listen/respond) | planned | P0 | Keep responses concise and low-latency |
| F-03 | LED state UX mapping (`idle/listen/process/respond/error`) | planned | P0 | Deterministic, no ambiguous intermediate states |
| F-04 | Intent/task router (local-first, cloud fallback) | planned | P1 | Confidence-gated execution path |
| F-05 | Face recognition memory assist (name reminders) | planned | P1 | Detect -> embed -> local match -> audio cue |
| F-06 | Unknown person onboarding flow | planned | P1 | Prompt for name + capture 3-5 samples |
| F-07 | Privacy controls for identity data | planned | P0 | Opt-in, local encrypted store, forget/wipe commands |
| F-08 | Personal assistant skills (reminder, quick note, status) | planned | P2 | Add incrementally after core reliability |

## Face Recognition Memory Assist (F-05/F-06/F-07)
### Intended UX
- Known face: "This is <name>."
- Optional context: "Met at <team/event>."
- Unknown face: ask whether to save new contact.

### Enrollment Flow
1. User confirms add-person mode.
2. Capture multiple face samples.
3. Ask for spoken name.
4. Store embedding profile locally.
5. Confirm with audio/LED feedback.

### Safety and Privacy Requirements
- Explicit opt-in before enabling recognition.
- Local-first inference and storage by default.
- No raw image retention by default.
- Voice commands: "forget <name>", "wipe all identities".
- Clear indication when recognition mode is active.

## Architecture Direction
- Event bus + strict runtime state machine:
  - `IDLE -> WAKE -> LISTEN -> PROCESS -> RESPOND -> IDLE`
- Sensor/model outputs feed events, not direct UI behavior.
- UART default stays event-only; debug streams are opt-in.

## Delivery Phases
### Phase A (Now): Input Reliability
- Fix tap/swipe arbitration and misclassification.
- Add regression gesture scripts + confusion matrix logging.

### Phase B: Assistant Shell
- Implement wake/listen/process/respond loop.
- Add LED/audio state signaling.

### Phase C: Identity Assist
- Add camera ingest path and face embedding/matching.
- Add known/unknown person interaction flows.
- Add privacy controls and data lifecycle commands.

### Phase D: Skills and Polish
- Add high-value assistant skills.
- Optimize latency, stability, and power behavior.

## Current Focus
- Active milestone: `Phase A`
- Definition of done for current milestone:
  - Swipe and tap events are mutually exclusive in runtime.
  - Tap count accuracy is stable across 1/2/3 test sets.
  - Swipe direction accuracy is stable across up/down test sets.
  - Results are documented per firmware revision.

## Change Log
- 2026-03-24: Created master plan; merged AI pin-style direction and face-recognition memory assist into one editable roadmap.
