# KIT_PSE84_AI Binaries

Tracked location for published firmware artifacts for this board.

## Expected artifacts
- `.elf`
- `.hex`
- `.bin`

## Publishing workflow
Use:

```bash
../tools/publish_binaries.sh /path/to/build/output
```

This copies supported files into this folder and updates `manifest.json`.
