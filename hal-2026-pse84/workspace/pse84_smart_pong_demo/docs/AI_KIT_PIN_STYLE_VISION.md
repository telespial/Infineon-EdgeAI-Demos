# AI KIT Pin-Style Vision (PSE84)

## Purpose
Define what we are building on KIT_PSE84_AI: a compact, always-available, screen-light AI companion inspired by the AI-pin product direction.

Inspiration reference:
- https://www.geeky-gadgets.com/apple-ai-pin-rumors/

## Product Direction
We are building a **pin-style communicator prototype** that is:
- Hands-free first (voice + simple gestures)
- Event-driven (short actions, short responses)
- Low-friction (minimal UI, mostly audio/status feedback)
- Context-aware (adapts behavior to mode, intent, and environment)

## What This Means on PSE84
Given current hardware and firmware paths in this repo, the practical implementation is:
- Radar gestures as primary quick input (`swipe up`, `swipe down`, `taps=1|2|3`)
- Voice/audio pipeline for prompts and confirmations
- LED state signaling for always-on glanceable feedback
- On-device intent state machine (`IDLE -> WAKE -> LISTEN -> PROCESS -> RESPOND`)
- Optional cloud assist only when needed (keep core loop local-first)

## MVP Experience
1. User performs wake gesture.
2. Device enters listen state and confirms with LED/audio cue.
3. User speaks a short command.
4. Device executes local action or cloud-assisted task.
5. Device responds with concise voice/status feedback.
6. Device returns to idle with low-power heartbeat behavior.

## Feature Pillars
- Input: radar gestures + voice trigger + physical fallback control.
- Intelligence: intent routing, task execution, confidence gating.
- Output: short speech responses + deterministic LED signals.
- Reliability: debounce/lockout windows, event cooldowns, fail-safe fallback commands.
- Privacy: keep sensitive inference/data local when possible.

## Engineering Goals
- Keep UART default output event-only for testability.
- Make swipe/tap recognition mutually exclusive in runtime arbitration.
- Track confusion matrix by firmware revision.
- Separate prototype UX behavior from low-level sensor debug streams.

## Out of Scope (Current Phase)
- Full smartphone replacement
- Rich visual UI dependence
- Broad open-domain autonomous behavior without explicit user intent

## Success Criteria
- Gesture interaction is repeatable across users (documented regression set)
- Voice command loop is stable end-to-end
- State transitions are predictable and observable
- Demo feels immediate, useful, and "pin-like" in day-to-day interactions
