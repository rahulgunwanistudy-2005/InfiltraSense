# InfiltraSense Wokwi Digital Twin

InfiltraSense is an interactive Wokwi model that demonstrates a trust-aware IV-site monitoring architecture. It simulates bioimpedance, tissue strain, temperature, motion, and electrode-contact quality, then applies signal-quality gates, guarded baseline acquisition, lightweight embedded classification, multimodal corroboration, temporal persistence, abstention, and alert latching.

This repository is an executable architecture demonstration. It is not clinically validated and is not a diagnostic device.

## Run in VS Code

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the [Wokwi Simulator extension](https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode).
2. Clone this repository and open the repository folder in VS Code.
3. Activate the Wokwi extension if prompted.
4. Open `diagram.json`.
5. Click the green play button, or open the Command Palette with `Shift+Command+P` and run `Wokwi: Start Simulator`.
6. Allow about five seconds of simulation time for baseline acquisition.

The Wokwi Serial Plotter opens automatically and shows impedance, strain, motion, and temperature relative to their own detection thresholds. See [SIGNAL-GRAPHS.md](SIGNAL-GRAPHS.md) for the trace meanings and the complete output decision path.

The committed `firmware.bin`, `firmware.elf`, and `iv-phantom.chip.wasm` files make the first run build-free.

The VS Code simulation also exposes the ESP32-S3 serial console at `rfc2217://localhost:4000`, which can be used for automated checks or an external serial monitor.

## Interactive scenarios

- **NORMAL / RESTORE** — settles and acquires a trusted baseline before returning to `NORMAL`.
- **MOVE ARM** — introduces motion artifacts and produces `WATCH` while inference is withheld.
- **LIFT ELECTRODE** — degrades contact and produces `SENSOR INVALID` without creating an IV alert.
- **CATHETER SHIFT** — raises mechanical strain but remains `WATCH` because independent corroboration is insufficient.
- **INFILTRATION** — gradually changes impedance and strain; persistent corroborated evidence progresses from `WATCH` to `CHECK IV`.
- **ACK / MUTE** — silences the buzzer while retaining the visible alert.
- **FREE EXPERIMENT** — enables direct adjustment of the phantom controls.

On the graph, `100` is the threshold for each sensor trace. An alert candidate requires both impedance traces and strain to cross 100 while motion remains below 100 and the sample remains trusted. The final `CHECK_IV` trace rises only after this evidence persists continuously for four seconds. Temperature is supporting context and never triggers an alert by itself.

See [START-HERE.md](START-HERE.md) for the detailed walkthrough and [VERIFICATION.md](VERIFICATION.md) for the completed checks.

## Architecture mapping

| Proposed hardware | Executed model |
|---|---|
| AD5940 bioimpedance front end | Synthetic low- and high-frequency impedance channels |
| Strain sensor | Synthetic tissue-expansion measurement |
| MAX30208 | Synthetic local-temperature measurement |
| LSM6DSV80X | Synthetic normalized motion severity |
| MAX78002 edge inference | ESP32-S3 execution surrogate |
| nRF5340 clinician communication | OLED, LEDs, buzzer, and serial evidence console |

The independent custom chip sends raw synthetic measurements to the ESP32-S3. It does not send scenario names or final decisions, so the firmware computes the displayed state from measurement quality and evidence.

## Rebuild

The application can be rebuilt with PlatformIO using `platformio.ini`. After a successful build, copy the generated `firmware.bin` and `firmware.elf` from `.pio/build/esp32-s3/` to the repository root.

If `iv-phantom.chip.c` changes, rebuild the custom chip with Wokwi CLI:

```sh
wokwi-cli chip compile iv-phantom.chip.c -o iv-phantom.chip.wasm
```

## Regression tests

Run the host-side safety-policy suite on macOS or Linux:

```sh
./tests/run-tests.sh
```

The suite compiles the actual `sketch.ino` policy code with lightweight hardware stubs and checks baseline handling, manual reacquisition settling, artifacts, contact failure, false-escalation controls, persistent infiltration, alert acknowledgement, recovery, and stream timeout.
