# InfiltraSense live signal graphs

The Wokwi Serial Plotter opens automatically when the simulation starts. Wait about five seconds of simulation time for the quiet-period and baseline sequence, then press the scenario buttons.

## Reading the graph

All sensor traces share a normalized **percent of threshold** scale. The horizontal `Threshold` trace stays at 100:

| Graph trace | Underlying measurement | 100 means |
|---|---|---|
| `LF_Z` | Low-frequency impedance decrease from baseline | An 8% decrease |
| `HF_Z` | High-frequency impedance decrease from baseline | A 3% decrease |
| `Strain` | Strain increase from baseline | A 4 percentage-point increase |
| `Motion` | Normalized motion severity | Motion reaches 0.35 and inference is withheld |
| `Temperature` | Absolute local-temperature change from baseline | A 0.8°C change; supporting context only |
| `Candidate` | All alert-candidate rules currently pass | The line rises to 110 |
| `CHECK_IV` | The persistent alert is latched | The line rises to 125 |

For example, `LF_Z = 150` means the low-frequency impedance decrease is 1.5 times its 8% threshold, or about a 12% decrease from baseline. The live OLED still shows the most important physical values: impedance change, strain change, local temperature, motion, quality, and the current decision.

## How the output is detected

The firmware does not alert from a single graph. It applies these steps:

1. **Trust the sample:** contact must be at least 0.65, values must be in range, motion must not exceed 0.35, and combined signal quality must be at least 0.70.
2. **Establish a baseline:** one second of quiet settling is followed by four seconds of guarded baseline acquisition.
3. **Find independent evidence:** both impedance channels must fall (`LF_Z` and `HF_Z` at or above 100) and strain must rise (`Strain` at or above 100).
4. **Check the model:** the demonstration classifier's site score must be at least 0.75 and it must not be uncertain.
5. **Require persistence:** `Candidate` must remain high for four continuous seconds. Any movement, contact failure, or loss of corroboration resets the timer.
6. **Latch the output:** `CHECK_IV` rises, the red LED and buzzer activate, and acknowledgement only mutes the sound. The visual alert stays until trusted recovery is confirmed.

Temperature is deliberately not a standalone trigger. It is graphed as context and is a weak classifier input, but temperature alone can only produce `WATCH`, not `CHECK IV`. Motion is also not infiltration evidence: when its trace crosses 100, inference is withheld to prevent movement artifacts from creating an alert.

## Expected scenario patterns

- **NORMAL / RESTORE:** all evidence traces remain below 100; output becomes `NORMAL` after baseline acquisition.
- **MOVE ARM:** `Motion` crosses 100; output is `WATCH`, with no candidate or alert.
- **LIFT ELECTRODE:** contact fails; output is `SENSOR INVALID` and inference is withheld.
- **CATHETER SHIFT:** strain can cross 100, but impedance does not corroborate it; output stays `WATCH`.
- **INFILTRATION:** both impedance traces and strain cross 100, `Candidate` rises, and `CHECK_IV` rises after four seconds of continuous evidence.

## Switching back to the evidence console

The plotter is the default view. To inspect detailed raw readings and policy receipts, switch the Wokwi serial panel from Plotter to Terminal and send `P`. Send `P` again to resume clean graph output. In terminal mode, `?` prints a receipt, `A` acknowledges the alarm, and `R` requests guarded baseline reacquisition.

This is an executable architecture demonstration using synthetic signals and a hand-authored classifier. It is not clinically validated and is not a diagnostic device.
