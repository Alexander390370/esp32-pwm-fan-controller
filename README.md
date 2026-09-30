# ESP32 PWM Fan Controller

A DC motor speed controller driven by an ESP32. The potentiometer sets the PWM duty cycle, an S8050 transistor handles the current, and an OLED shows the live duty cycle.

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue)

## Overview

Driving a DC motor directly from an ESP32 GPIO will fry the pin. This project shows the standard fix: a small-signal NPN transistor (S8050) as a low-side switch, with a flyback diode across the motor to absorb the back-EMF when the motor shuts off. The potentiometer gives smooth, analog-style speed control from 0 to 100%.

## Hardware Bill of Materials

| Component | Model | Qty |
|-----------|-------|-----|
| MCU | ESP32 (or ESP32-S3) | 1 |
| DC motor | Small brushed motor (5V, < 500mA) | 1 |
| NPN transistor | S8050 | 1 |
| Flyback diode | 1N4148 (small motors) / 1N4007 (larger) | 1 |
| Base resistor | 220Ω | 1 |
| Potentiometer | 10kΩ linear | 1 |
| OLED display | SSD1306 (I2C, 128x64) | 1 |
| External supply | 5V (e.g. HW-131 battery box) | 1 |

> ⚠️ **S8050 is rated for small 5V motors only.** Above roughly 500mA continuous, it will overheat and fail. For bigger loads, use a logic-level power MOSFET (e.g. IRL540N).

## Pin Mapping

| Component | Pin | ESP32 GPIO |
|-----------|-----|------------|
| Potentiometer | Wiper (middle) | P34 |
| Potentiometer | Left / Right | 3.3V / GND |
| Transistor base | via 220Ω resistor | P27 |
| OLED | SDA | P21 |
| OLED | SCL | P22 |
| Motor | + | 5V (external) |
| Motor | − | Transistor collector |
| Transistor emitter | — | GND (common with ESP32) |
| Flyback diode | — | Reverse-biased across motor terminals |

> ⚠️ **The base resistor must be 220Ω, not 1kΩ.** With 1kΩ, the transistor stays in the linear region, gets very hot, and delivers very little torque to the motor. 220Ω forces it into saturation.

> 💡 P34 is used here because GPIO34–39 on the ESP32 are **input-only** and have no internal pull-ups — ideal for analog input, not usable for output.

## Wiring Notes

```text
Motor + ── 5V (external)
Motor − ── Transistor collector (C)
Transistor emitter (E) ── GND (shared with ESP32)
Transistor base (B) ── 220Ω ── ESP32 P27
1N4148 diode: cathode (silver stripe) to Motor +, anode to Motor −
```

**Common ground is mandatory.** The external 5V supply's GND and the ESP32's GND must be tied together, otherwise the transistor's base current has no return path and the motor won't spin.

## Code

See [`fan_controller.ino`](./fan_controller.ino) for the full source.

The sketch reads the potentiometer via `analogRead()`, maps 0–4095 to a PWM duty cycle 0–255, and drives the transistor with `ledcWrite()`. PWM is configured at 1 kHz / 8-bit resolution — a good balance for small motors. The OLED shows the raw ADC value and the current duty cycle.

## Quick Start

```text
1. Wire the circuit per the pin table above.
2. Open fan_controller.ino in Arduino IDE.
3. Install libraries via Sketch → Include Library → Manage Libraries:
     - Adafruit SSD1306
     - Adafruit GFX
4. Select "ESP32 Dev Module" (or your specific board) from Tools → Board.
5. Select the correct COM port, then upload.
```

**Expected result**: rotate the potentiometer — the OLED shows the ADC value and PWM duty cycle, and the motor speed changes smoothly from 0% to 100%.

> 💡 If the OLED stays blank, check the I2C address. Most SSD1306 modules are `0x3C`, some are `0x3D`. Run an I2C scanner sketch to confirm.

## Engineering Pitfalls

1. **Never drive a motor directly from GPIO.** The pin current limit is around 40mA; motors draw hundreds of mA and will damage the MCU.
2. **Flyback diode is not optional.** When the motor stops, its inductance produces a voltage spike that will kill the transistor. 1N4148 across the motor terminals absorbs it (use 1N4007 for higher-current motors).
3. **Base resistor value matters.** 220Ω for saturation; 1kΩ turns the transistor into a heater.
4. **Common ground.** External supply and ESP32 must share GND.
5. **Potentiometer wiring.** Only the wiper goes to the ADC pin. Connecting either end to 3.3V *and* the wiper to GND will short the rail through the pot.
6. **Audible motor whine.** If the motor emits a high-pitched sound, increase the LEDC PWM frequency (e.g. from 1 kHz to 20 kHz) in `ledcSetup()` to move it out of the audible range.

## License

Distributed under the MIT License. See [LICENSE](./LICENSE) for details.
