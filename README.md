# OBV Robot 🤖

An Arduino-powered **Object-aVoidance robot** with three switchable control modes, driven by a custom Flutter Android app featuring a live sonar-style radar HUD.

> **Manual** · **Voice** · **Autonomous obstacle avoidance** — all over Bluetooth.

<!-- TODO: add a demo GIF / photo of the robot here -->

## ✨ Features

- **🎮 Manual mode** — drive with an on-screen D-pad + speed control.
- **🎙️ Voice mode** — speak commands ("forward", "left", "stop"); the phone maps speech to commands.
- **🤖 Auto mode** — the robot sweeps its ultrasonic sensor and avoids obstacles on its own.
- **📡 Live radar HUD** — real ultrasonic telemetry rendered as a glowing sonar sweep.
- One simple command protocol shared by all three modes.

## 🧩 Hardware

| Component | Role |
|-----------|------|
| Arduino Uno | Main controller |
| L293D Motor Driver Shield | Drives the 4 DC motors (`AFMotor` library) |
| HC-05 Bluetooth module | Bluetooth **Classic** (SPP) link to the phone |
| SG90 servo | Sweeps the ultrasonic sensor (pin 9) |
| HC-SR04 ultrasonic sensor | Distance sensing (`trig=A1`, `echo=A2`) |
| 4× DC motors | Drive |
| 3× 18650 Li-ion (series) | ~11.1–12.6V power |
| Buck converter (LM2596) | Clean 5V supply — see [hardware notes](docs/hardware-notes.md) |

Full pin map: [`docs/wiring.md`](docs/wiring.md).

## 📟 Command protocol

The app and firmware talk over a tiny single-character serial protocol.

**App → Arduino**

| Char | Action |
|------|--------|
| `F` / `U` | Forward |
| `B` / `D` | Backward |
| `L` | Pivot left |
| `R` | Pivot right |
| `S` | Stop (also exits auto mode) |
| `T` | Enter obstacle-avoidance mode |
| `0`–`9` | Set speed (0–255) |

**Arduino → App (telemetry)**

```
A<angle>D<distance>\n     e.g. A90D42   (servo at 90°, obstacle 42 cm away)
```

## 📁 Structure

```
OBV-Robot/
├── firmware/obv_robot/     Arduino sketch (3 modes + telemetry)
├── app/obv_control/        Flutter Android app
└── docs/                   hardware notes & wiring
```

## 🚀 Getting started

### Firmware
1. Install the **`Adafruit Motor Shield`** (`AFMotor`) library in the Arduino IDE.
2. Open `firmware/obv_robot/obv_robot.ino`.
3. **Unplug the HC-05** (it shares the serial pins used for upload), then flash the Uno.
4. Reconnect the HC-05.

### App
1. Install [Flutter](https://docs.flutter.dev/get-started/install) and Android Studio.
2. From `app/obv_control/`, run `flutter pub get` then `flutter run` on your Android phone.
3. Pair your phone with the HC-05 in Android Bluetooth settings first (PIN usually `1234` / `0000`).

## 🗺️ Roadmap

- [x] Upgraded firmware: non-blocking 3-mode + telemetry
- [x] Flutter app: Bluetooth connection + live radar
- [x] Manual, Voice, and Auto mode UIs
- [x] "Sonar HUD" theme
- [ ] On-device testing with the robot
- [ ] Untethered power fix (buck converter)

## 📝 Notes

The onboard 5V regulator on this Uno is faulty on battery power — a buck converter provides a clean 5V. The full diagnosis is documented in [`docs/hardware-notes.md`](docs/hardware-notes.md).
