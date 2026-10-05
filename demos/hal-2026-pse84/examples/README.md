# KIT_PSE84_AI Examples

Tracked example application sources for the Infineon KIT_PSE84_AI workflow.

## Included
- `pse84_cli_blinky/`: minimal CLI-first starter app source for bring-up.
- `pse84_rgb_alive/`: tri-color RGB breathing + glitch animation ("alive / breaking" effect).

## Build Model
- Build is intended through Infineon ModusToolbox command-line tooling.
- Use `../tools/create_pse84_mtb_app.sh` to scaffold an app workspace when `project-creator-cli` is available.
- Produced artifacts (`.elf/.hex/.bin`) should be copied into `../binaries/` using `../tools/publish_binaries.sh`.

## Known Hardware Note (RGB LED3)
- On this specific KIT_PSE84_AI board under test, LED3 did not expose an independently visible blue channel during direct GPIO tests.
- Observed mapping during raw pin drive tests:
  - `P20_4` drove red
  - `P20_5` drove green
  - `P20_6` produced bright green (not blue)
- Current demo fallback uses a red/green breathing palette and keeps `P20_6` off for stable visual behavior.
