# E84 AI Kit 7-inch Smart Pong Quick Test

## Hardware
- Board: KIT_PSE84_AI
- Display: Waveshare 7-inch Raspberry Pi DSI LCD (C), 1024x600

## Wiring
- Connect 15-pin DSI cable to board connector `J10`.
- If your panel/touch path needs external 5V + I2C touch, ensure `J16` is populated.

## App Defaults
- `TARGET=KIT_PSE84_AI`
- `CONFIG_DISPLAY=WS7P0DSI_RPI_DISP`

## Build and Flash
From this app directory:

```bash
make getlibs
make build TOOLCHAIN=GCC_ARM TARGET=KIT_PSE84_AI CONFIG_DISPLAY=WS7P0DSI_RPI_DISP -j8
make program TOOLCHAIN=GCC_ARM TARGET=KIT_PSE84_AI CONFIG_DISPLAY=WS7P0DSI_RPI_DISP
```

Or use helper scripts after `docs/PROJECT_STATE.md` command verification:

```bash
./tools/build.sh
./tools/flash.sh
```

## Expected Result
- UART boot logs appear.
- Smart Pong UI renders on the 7-inch panel.
- Touch strips on left/right edges control paddles.
