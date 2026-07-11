# Wiring / Pin Map

## Arduino Uno + L293D Motor Shield

The L293D shield stacks directly on the Uno and uses the `AFMotor` library. Motor and servo connections are on the shield; the ultrasonic sensor uses the shield's broken-out analog pins.

### Motors (screw terminals on the shield)

| Shield port | Motor | Position |
|-------------|-------|----------|
| M1 | motor1 | Right Front |
| M2 | motor2 | Left Front |
| M3 | motor3 | Left Rear |
| M4 | motor4 | Right Rear |

### Servo

| Signal | Pin |
|--------|-----|
| SG90 signal | Servo header **`SER1`** → pin **9** |
| SG90 V+ / GND | Servo header 5V / GND |

### Ultrasonic sensor (HC-SR04)

| Sensor pin | Arduino pin |
|------------|-------------|
| Trig | **A1** |
| Echo | **A2** |
| VCC | 5V |
| GND | GND |

### HC-05 Bluetooth (Classic / SPP)

| HC-05 pin | Arduino pin | Note |
|-----------|-------------|------|
| VCC | 5V | From the buck converter's clean 5V |
| GND | GND | |
| TXD | RX (pin 0) | HC-05 TX → Uno RX |
| RXD | TX (pin 1) | Uno TX → HC-05 RX (a voltage divider on this line is recommended; HC-05 RX is 3.3V) |

> ⚠️ **Upload note:** the HC-05 shares the hardware serial pins (0/1) used for USB flashing. **Unplug the HC-05 while uploading**, then reconnect.

## Power

See [`hardware-notes.md`](hardware-notes.md) for the full power path and the buck-converter fix.

```
Battery(+) ──────────────────┬───────────► Barrel jack (+) ──► motors (Vin)
                             └──► Buck IN+ / OUT+ ──► Arduino 5V pin
Battery(−) ──►[SWITCH]───────┬───────────► Barrel jack (−)
                             └──► Buck IN− / OUT− ──► Arduino GND
```

- 3× 18650 in **series** → ~11.1–12.6V.
- Master switch on the **negative** line.
- Buck converter set to **exactly 5.0V** feeds the Arduino `5V` pin (bypasses the faulty onboard regulator).
