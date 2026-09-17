# Verification record — 2026-09-08

## Reverification — 2026-09-17

- Added automatic live Wokwi Serial Plotter graphs for low- and high-frequency impedance, strain, motion, and temperature, all normalized to a common threshold scale.
- Added `Candidate` and `CHECK_IV` traces so the four-second persistence step and final latched output are directly visible.
- Added regression checks for the graph packet format and for normal, motion, temperature-only, and infiltration graph behavior.
- Rebuilt the ESP32-S3 firmware successfully from the repository source.
- Recompiled the custom phantom source successfully to WebAssembly.
- Revalidated `diagram.json`: zero Wokwi lint errors.
- Added a reproducible host-side policy suite under `tests/`; all 15 checks pass.
- Fixed manual `R` reacquisition so it resets the quiet timer and enforces one full second of artifact-free settling before baseline samples are accepted.
- Removed a signed/unsigned UART-frame length comparison warning found by the strict host build.
- Added an RFC2217 serial bridge on port 4000 for live local-runtime verification.
- Started the prebuilt project through the installed VS Code Wokwi extension and confirmed that the simulator opened port 4000 and emitted live labeled graph packets at runtime.
- The public browser project reached Wokwi successfully, but its source build was blocked by Wokwi's `Build Servers Busy` queue. The VS Code project uses committed prebuilt firmware and WASM files and does not depend on that browser build queue.

## Completed checks

- ESP32-S3 firmware rebuilt successfully with PlatformIO 6 / Espressif32 6.10.0.
- Custom IV-site phantom compiled successfully from C to WebAssembly with Wokwi CLI 0.26.1.
- `diagram.json` linted with Wokwi CLI: zero errors. Two invalid `3V3` pin names from the earlier package were corrected to `3V3.1` and `3V3.2`.
- VS Code opened the folder and Wokwi Simulator 3.7.0 started the model successfully from `diagram.json` and `wokwi.toml`.
- The running firmware produced clean plotter packets containing `LF_Z`, `HF_Z`, `Strain`, `Motion`, `Temperature`, `Threshold`, `Candidate`, and `CHECK_IV` fields.
- Native policy harness passed all behavioral checks:
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

## External service note

The optional Wokwi CLI cloud-simulation command requires a separate `WOKWI_CLI_TOKEN`; this is not needed for the verified VS Code workflow. The local VS Code simulation is activated and starts successfully. Open `diagram.json`, press the green play button, and follow `START-HERE.md` for the scenario walkthrough.
