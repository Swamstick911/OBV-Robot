# Hardware Notes

Running log of hardware issues, diagnoses, and fixes for the OBV Robot.

## Components

| Part | Notes |
|------|-------|
| Arduino Uno | Main controller. **Onboard 5V regulator is faulty** — see issue below. |
| L293D Motor Driver Shield | Uses the `AFMotor` library. Drives 4 DC motors. |
| HC-05 Bluetooth module | Bluetooth **Classic** (SPP). Powered from the 5V rail. |
| SG90 servo | Sweeps the ultrasonic sensor. On pin 9. |
| HC-SR04 ultrasonic sensor | `trigPin = A1`, `echoPin = A2`. |
| 4x DC motors | M1 = Right Front, M2 = Left Front, M3 = Left Rear, M4 = Right Rear. |
| 3x 18650 Li-ion cells | ~2200mAh, 3.7V each, wired in **series** (~11.1–12.6V). Note: 2 pink + 1 blue = mixed cells; ideally match them. |

---

## Issue #1 — Robot works on USB but HC-05 stays dead on battery (RESOLVED: diagnosis)

### Symptom
- On **laptop USB**: everything works, HC-05 blinks and connects.
- On **battery**: Arduino "ON" LED and shield LED light up, but the **HC-05 never powers on** (no LED).

### Diagnosis journey
1. Ruled out "won't power on" — the board *does* power up on battery (LEDs on). So it powers up but misbehaves.
2. Measured the **battery pack: 11.85V** at the switch connectors → healthy series pack, fully charged. (An earlier 3.97V reading was a single cell measured by mistake.)
3. Found and cleaned **glue on the power switch connectors** — a suspected bad contact, but not the root cause.
4. Measured **Vin → GND = ~11.85V** (good) but the **5V rail = ~2.5V** (should be 5V).
5. Confirmed **not a short circuit**: on USB the 5V rail is fine, so the load is normal.

### Root cause
**The Arduino Uno's onboard 5V regulator is dead/weak.** Given a healthy 11.85V on Vin it only produces ~2.5V on the 5V rail. USB "worked" all along because USB feeds 5V directly to the rail, bypassing the broken regulator. Common failure on cheap Uno clones (over-voltage or motor/servo current spikes).

### Fix
Add a **DC-DC buck converter** (LM2596 or MP1584):
1. **Set the buck output to exactly 5.0V first** (adjust with a multimeter attached — feeding >5V into the `5V` pin can damage the board).
2. Wire **battery +/− → buck IN+/IN−**.
3. Wire **buck OUT+ → Arduino `5V` pin**, **buck OUT− → Arduino `GND`**.
4. Motors continue to run off the battery via the barrel jack (Vin) — unaffected by the dead regulator.

This gives a strong, clean 5V for the logic + HC-05 + servo, bypassing the faulty onboard regulator.

### Buck converter wiring (this build: switch is on the NEGATIVE line)

The master switch is wired in the **battery negative (−) line**. The buck is added **in parallel** with the barrel jack, both fed from *after* the switch, so one flip cuts the whole robot.

```
Battery(+) ──────────────────┬───────────► Barrel jack (+) ──► motors (Vin)
                             └──► Buck IN+
                                  Buck OUT+ ──► Arduino 5V pin
Battery(−) ──►[SWITCH]───────┬───────────► Barrel jack (−)
                             └──► Buck IN−
                                  Buck OUT− ──► Arduino GND
```

- **Buck `IN+`** → battery positive line (same node as barrel jack +).
- **Buck `IN−`** → switched negative (the wire after the switch, same node as barrel jack −).
- **Buck `OUT+/OUT−`** → Arduino `5V` / `GND` (after setting output to exactly 5.0V).
- Barrel jack stays connected as-is for motor power.

**Sanity check:** switch OFF → buck input reads 0V; switch ON → ~11.85V. Confirms the buck is after the switch.

**Status:** Fix identified; buck converter to be purchased and installed. Interim workaround: a USB power bank into the Arduino's USB port powers the logic/HC-05 (battery still drives the motors).

---

## Upload reminder
The HC-05 sits on the Uno's hardware serial (pins 0/1) — the same pins used for USB uploads. **Unplug the HC-05 while flashing the sketch**, then reconnect it.
