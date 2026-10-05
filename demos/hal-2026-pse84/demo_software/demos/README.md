# KIT_PSE84_AI Demo Bundle

Pulled official Infineon PSOC Edge demo applications as git submodules.

## Included demos
- mtb-example-psoc-edge-hello-world
- mtb-example-psoc-edge-empty-app
- mtb-example-psoc-edge-adc-basic
- mtb-example-psoc-edge-pwm-timer
- mtb-example-psoc-edge-spi-dma
- mtb-example-psoc-edge-ipc-pipes
- mtb-example-psoc-edge-btstack-findme
- mtb-example-psoc-edge-wlan-offloads
- mtb-example-psoc-edge-otw-update
- mtb-example-psoc-edge-ml-deepcraft-deploy-vision
- mtb-example-psoc-edge-ae-application

## Update demos
```bash
git submodule update --init --recursive
git submodule update --remote --recursive
```

## Notes
- These are upstream Infineon repos; build/tooling requirements are defined in each demo README.
- Use KIT_PSE84_AI target selection in each app where applicable.
