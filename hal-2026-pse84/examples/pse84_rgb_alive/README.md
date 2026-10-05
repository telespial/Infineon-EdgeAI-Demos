# PSE84 RGB Alive

Tricolor LED animation for KIT_PSE84_AI that blends:
- breathing color drift
- uneven phase movement
- short glitch bursts ("alive / breaking" effect)

## Behavior
- Runs on `CYBSP_LED_RGB_RED`, `CYBSP_LED_RGB_GREEN`, `CYBSP_LED_RGB_BLUE`.
- Uses software PWM so it can be dropped into a minimal app without extra PWM routing setup.

## Integration
- Copy `main.c` into a ModusToolbox app project targeting `KIT_PSE84_AI`.
- Build/flash with your existing CLI flow.

## Tuning
- Edit spike frequency: `(rnd & 0x003Fu) == 0u`
- Edit base speed: increments of `pr/pg/pb`
- Edit intensity mix: constants near `80/96/112`

## Known Issue (Current Board Under Test)
- Schematic/user-guide indicate LED3 is RGB, but direct GPIO validation on this board did not show an independent blue output.
- Raw-drive observations:
  - `P20_4` -> red
  - `P20_5` -> green
  - `P20_6` -> bright green (not blue)
- For now, firmware in the active workspace is using a red/green breathing palette as a practical fallback.
