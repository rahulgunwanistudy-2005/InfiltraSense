# InfiltraSense — verified VS Code start guide

This folder contains a prebuilt ESP32-S3 firmware image and the compiled WebAssembly model for the interactive IV-site phantom. You do **not** need to build anything for the first run.

## First run

1. Move **Visual Studio Code** from Downloads to Applications. macOS is currently running it through App Translocation, which can make extension behavior confusing.
2. Open this entire `InfiltraSense-VSCode` folder in VS Code. Do not open only `sketch.ino`.
3. Install the recommended **Wokwi Simulator** extension if VS Code asks. It is already installed in the VS Code profile used during verification.
4. Open `diagram.json` from the Explorer sidebar.
5. Click the green play button at the upper-left of the diagram view. Alternatively, press `Shift+Command+P`, type `Wokwi: Start Simulator`, and press Return.
6. If Wokwi asks for activation, follow its sign-in/trial prompt. Local simulation requires Wokwi activation.
7. Wait about five seconds of **simulation time** for baseline acquisition. Then press the scenario buttons in the diagram.

## What should happen

- `NORMAL / RESTORE`: after settling and baseline acquisition, OLED reads `NORMAL`; green LED is on.
- `MOVE ARM`: OLED reads `WATCH`; inference is withheld because of motion.
- `LIFT ELECTRODE`: OLED reads `SENSOR INVALID`; no IV alert is created.
- `CATHETER SHIFT`: OLED reads `WATCH`; one modality is insufficient.
- `INFILTRATION`: transitions through `WATCH`, then reaches `CHECK IV` after persistent corroborated evidence; red LED and buzzer activate.
- `ACK / MUTE`: silences the buzzer but keeps the visual alert latched.

## Important corrections to the earlier instructions

- The browser command `Upload Firmware and Start Simulation...` is **not** the normal VS Code workflow.
- VS Code requires `wokwi.toml`; it was missing before.
- A local custom chip requires a compiled `.wasm` binary; the earlier package only described the `.chip.c` source.
- `F1` is unnecessary on a Mac. Use `Shift+Command+P` for the Command Palette.
- Open the whole folder so `wokwi.toml`, `diagram.json`, firmware, and chip files share one workspace root.

## Rebuilding after code changes (optional)

The included firmware already runs. Only rebuild if you edit `sketch.ino`:

1. Install the PlatformIO extension recommended by this folder.
2. Use PlatformIO's **Build** action.
3. Copy `.pio/build/esp32-s3/firmware.bin` and `.pio/build/esp32-s3/firmware.elf` to the project root, replacing the existing two files.
4. Restart the Wokwi simulation.

If you edit `iv-phantom.chip.c`, recompile it with Wokwi CLI:

```text
wokwi-cli chip compile iv-phantom.chip.c -o iv-phantom.chip.wasm
```

This is an executable digital twin for architecture demonstration only. It is not clinically validated and is not a diagnostic device.
