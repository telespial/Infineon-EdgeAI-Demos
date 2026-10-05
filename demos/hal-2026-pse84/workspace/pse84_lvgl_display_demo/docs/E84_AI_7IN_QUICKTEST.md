# E84 AI Kit 7-inch DSI Quick Test

## Hardware
- Display: Waveshare 7-inch Raspberry Pi DSI LCD (C), 1024x600
- Board: KIT_PSE84_AI

## Connections
- Connect display 15-pin DSI FPC to `J10` (MIPI-DSI) on KIT_PSE84_AI.
- Ensure `J16` header is populated if your display/touch variant requires external 5V + I2C touch.

## Build Configuration
This project is preconfigured in `common.mk` for 7-inch test:
- `TARGET=APP_KIT_PSE84_AI`
- `CONFIG_DISPLAY = WS7P0DSI_RPI_DISP`

## Build and Flash
From this app root:

```bash
make getlibs
make build TOOLCHAIN=GCC_ARM -j8
make program TOOLCHAIN=GCC_ARM
```

If needed, you can also force target/display on the command line:

```bash
make build TOOLCHAIN=GCC_ARM TARGET=APP_KIT_PSE84_AI CONFIG_DISPLAY=WS7P0DSI_RPI_DISP -j8
make program TOOLCHAIN=GCC_ARM TARGET=APP_KIT_PSE84_AI CONFIG_DISPLAY=WS7P0DSI_RPI_DISP
```

## Expected Result
- UART banner appears for the LVGL demo.
- The panel shows the LVGL music player demo UI.
- Touch input responds (if touch wiring/power path is enabled for your panel variant).
