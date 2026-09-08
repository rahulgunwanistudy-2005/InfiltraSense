# Verification record — 2026-09-08

## Completed checks

- ESP32-S3 firmware rebuilt successfully with PlatformIO 6 / Espressif32 6.10.0.
- Custom IV-site phantom compiled successfully from C to WebAssembly with Wokwi CLI 0.26.1.
- `diagram.json` linted with Wokwi CLI: zero errors. Two invalid `3V3` pin names from the earlier package were corrected to `3V3.1` and `3V3.2`.
- VS Code 1.108.0 opened the folder and the installed Wokwi Simulator 3.7.0 extension recognized `diagram.json` and `wokwi.toml`.
- Native policy harness passed all 14 behavioral checks:
  - normal boot/baseline
  - motion rejection
  - electrode-contact rejection
  - guarded reacquisition
  - catheter shift does not falsely escalate
  - infiltration persistence and alert
  - acknowledgement preserves the active visual alert
  - contact fault retains an existing alert
  - abnormal site cannot become a new baseline
  - acknowledged recovery clears after washout/reacquisition
  - independent strain alone does not escalate
  - temperature alone does not escalate
  - interrupted evidence resets persistence
  - sensor-stream timeout becomes invalid

## Remaining runtime gate

VS Code displayed `Missing Wokwi license key!`. The simulator cannot start until the local Wokwi extension is activated for the user's account. This is the only uncompleted runtime check; it is an account/licensing gate rather than a project-file error.

After activation, open `diagram.json` and press the green play button. Follow `START-HERE.md` for the scenario walkthrough.
