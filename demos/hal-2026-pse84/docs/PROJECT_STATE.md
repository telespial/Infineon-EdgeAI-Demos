# Project State

Last updated: 2026-03-24

## Updates
- 2026-03-24: Repository synced from origin via git clone.
- 2026-03-24: Imported LVGL MIPI-DSI demo from PSOC_EDGE_E8_EVAL (`Infineon_e8_eval_sourcecode`) into `workspace/pse84_lvgl_display_demo` for Raspberry Pi-compatible display testing.
- 2026-03-24: Set demo defaults in `workspace/pse84_lvgl_display_demo/common.mk` to `TARGET=APP_KIT_PSE84_AI` and `CONFIG_DISPLAY = WS7P0DSI_RPI_DISP` (7-inch profile).
- 2026-03-24: Local build attempt blocked by missing ModusToolbox tool path setup (`common_app.mk: Unable to find any of the available CY_TOOLS_PATHS`).
- 2026-03-24: Added 7-inch display bring-up guide at workspace/pse84_lvgl_display_demo/docs/E84_AI_7IN_QUICKTEST.md (J10/J16 wiring + build/flash commands).
- 2026-03-24: Ported Smart Pong app from E8 eval source into `workspace/pse84_smart_pong_demo` for E84 AI kit.
- 2026-03-24: Retargeted Smart Pong defaults to `TARGET=KIT_PSE84_AI` and `CONFIG_DISPLAY=WS7P0DSI_RPI_DISP` for 7-inch DSI panel bring-up.
- 2026-03-24: Added 7-inch Smart Pong quick-test guide at `workspace/pse84_smart_pong_demo/docs/E84_AI_7IN_SMART_PONG_QUICKTEST.md`.
- 2026-03-24: Added `workspace/mtb_shared` assets via getlibs flow for imported MTB apps in this repo.
- 2026-03-24: Smart Pong build currently blocked: `proj_cm33_s/libs/mtb.mk` remains minimal after `make getlibs`, and build reports missing libraries `bsp core-make recipe-make`.
- 2026-03-24: No writes were made to `PSOC_EDGE_E8_EVAL`; it was used strictly as read-only source for copying/reference.
- 2026-03-24: Patched `workspace/pse84_smart_pong_clean/proj_cm33_s|proj_cm33_ns|proj_cm55/libs/mtb.mk` to map `SEARCH_TARGET_APP_KIT_PSE84_AI` to `../bsps/TARGET_APP_KIT_PSE84_AI`; this unblocked missing-base-library failures and allowed all three projects to compile/link.
- 2026-03-24: Full application `make build` still fails post-build at `sign_combine` because EdgeProtect Secure Suite is not installed (`Error: EdgeProtect Secure Suite not found`).
- 2026-03-24: Tried CM55-only flashing path (`make -C proj_cm55 qprogram_proj ...`) to bypass combine-sign; OpenOCD launched but programming failed with `unable to find a matching CMSIS-DAP device`.
- 2026-03-24: Retried with KitProg3 connected; `make -C proj_cm55 qprogram_proj TOOLCHAIN=GCC_ARM TARGET=APP_KIT_PSE84_AI CONFIG_DISPLAY=WS7P0DSI_RPI_DISP` succeeded. OpenOCD programmed and verified `proj_cm55.hex` (about 1.06 MB) on APP_KIT_PSE84_AI.
- 2026-03-24: Identified root issue for full-image flashing: EdgeProtect suite was installed but not auto-detected by build system.
- 2026-03-24: Executed full signed/combined flash successfully by setting `CY_TOOL_edgeprotecttools_EXE_ABS` to the installed EdgeProtect binary path and running `make program ...`; `app_combined.hex` programmed and verified (about 1.11 MB). This updates CM33 secure/non-secure and CM55 images together.
- 2026-03-24: Restored board to radar demo baseline by creating `workspace/pse84_radar_restore_app` from official template `mtb-example-psoc-edge-ml-deepcraft-deploy-radar` for `KIT_PSE84_AI`.
- 2026-03-24: Built and flashed `workspace/pse84_radar_restore_app` with explicit `CY_TOOL_edgeprotecttools_EXE_ABS`; full signed `app_combined.hex` programmed and verified successfully (about 0.85 MB).
- 2026-03-24: Re-ran radar restore with explicit `make erase` followed by full `make program`; OpenOCD erase/program/verify all succeeded again for `workspace/pse84_radar_restore_app/build/app_combined.hex`.
- 2026-03-24: Patched `workspace/pse84_radar_restore_app/proj_cm55/main.c` to event-only UART gesture output (`swipe up`, `swipe down`, `taps=1..3`) by mapping DeepCraft radar model classes (`Circle`, `Push`) into baseline-style events with debounce/grouping logic.
- 2026-03-24: Rebuilt and flashed patched `workspace/pse84_radar_restore_app` using explicit `CY_TOOLS_PATHS`, `CY_COMPILER_GCC_ARM_DIR`, and `CY_TOOL_edgeprotecttools_EXE_ABS`; OpenOCD write/verify succeeded for `build/app_combined.hex` (about 0.85 MB).
- 2026-03-24: Note: This is a compatibility shim on top of the available `Circle/Push` model, not the original `xensiv_radar_gestures` middleware pipeline.
- 2026-03-24: User reported patched image mismatch (green LED off). Created and flashed clean, unmodified official radar template to `workspace/pse84_radar_official_clean`.
- 2026-03-24: Full signed program of `workspace/pse84_radar_official_clean/build/app_combined.hex` succeeded (write+verify complete; ~0.85 MB).
- 2026-03-24: Added startup indicator in `workspace/pse84_radar_official_clean/proj_cm55/main.c` to force `CYBSP_LED_RGB_GREEN` ON immediately after `cybsp_init()`.
- 2026-03-24: Reflashed `workspace/pse84_radar_official_clean/build/app_combined.hex` after green-LED patch; OpenOCD write+verify succeeded.
