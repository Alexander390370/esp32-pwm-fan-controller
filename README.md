# ESP32 PWM Fan Controller with Web UI

A DC motor speed controller driven by an ESP32, combining analog PWM control with a built-in web interface. The potentiometer sets the motor speed; a web page toggles a separate LED; the OLED shows live status including the device's IP address.

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue)

## Overview

Driving a DC motor directly from an ESP32 GPIO will fry the pin. This project shows the standard fix: a small-signal NPN transistor (S8050) as a low-side switch, with a flyback diode across the motor to absorb the back-EMF when the motor shuts off.

On top of that, it adds a **standalone web UI** served directly from the ESP32 (no external server required), letting you toggle a separate LED from any browser on the same network. The OLED displays the current IP, live PWM duty cycle, and web-controlled LED state.

This makes it a compact demo of three things at once: **power electronics, web-served control, and hardware status feedback.**

## Hardware Bill of Materials

| Component | Model | Qty |
|-----------|-------|-----|
| MCU | ESP32 (or ESP32-S3) | 1 |
| DC motor | Small brushed motor (5V, < 500mA) | 1 |
| NPN transistor | S8050 | 1 |
| Flyback diode | 1N4148 (small motors) / 1N4007 (larger) | 1 |
| Base resistor | 220Ω | 1 |
| Potentiometer | 10kΩ linear | 1 |
| Indicator LED | 5mm LED + 220Ω current-limiting resistor | 1 |
| OLED display | SSD1306 (I2C, 128x64) | 1 |
| External supply | 5V (e.g. HW-131 battery box) | 1 |

> ⚠️ **S8050 is rated for small 5V motors only.** Above roughly 500mA continuous, it will overheat and fail. For bigger loads, use a logic-level power MOSFET (e.g. IRL540N).

## Pin Mapping

| Component | Pin | ESP32 GPIO |
|-----------|-----|------------|
| Potentiometer | Wiper (middle) | P34 |
| Potentiometer | Left / Right | 3.3V / GND |
| Transistor base | via 220Ω resistor | P27 |
| Indicator LED | via 220Ω resistor | P26 |
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

LED anode ── 220Ω ── ESP32 P26
LED cathode ── GND
```

**Common ground is mandatory.** The external 5V supply's GND and the ESP32's GND must be tied together, otherwise the transistor's base current has no return path and the motor won't spin.

## Repository Structure

```text
esp32-pwm-fan-controller/
├── README.md
├── LICENSE
├── .gitignore
├── fan_controller.ino
└── config.example.h
```

## Setup

### 1. Configure WiFi

Copy `config.example.h` to `config.h` and fill in your WiFi credentials:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

> **Do not commit `config.h`.** It is listed in `.gitignore` and will not be pushed to GitHub.

### 2. Wire the circuit

Follow the pin table and wiring notes above.

### 3. Flash the ESP32

```text
1. Open fan_controller.ino in Arduino IDE.
2. Install libraries via Sketch → Include Library → Manage Libraries:
     - Adafruit SSD1306
     - Adafruit GFX
4. Select "ESP32 Dev Module" (or your specific board) from Tools → Board.
5. Select the correct COM port, then upload.
```

## Usage

### Local control (potentiometer)

Rotate the potentiometer — the OLED shows the ADC value and PWM duty cycle, and the motor speed changes smoothly from 0% to 100%.

### Web control (LED toggle)

After boot, the OLED displays the ESP32's assigned IP (e.g. `192.168.1.42`). Open that IP in any browser on the same network:

```text
http://<ESP32_IP>/
```

You'll see a simple control page with two buttons (LED ON / LED OFF). Clicking them triggers `fetch()` requests to `/led/on` and `/led/off` — the ESP32 drives P26 accordingly, and the page updates its status text.

The OLED also reflects the LED state (`Web LED: ON/OFF`) so you can verify the command landed.

## Code Overview

The sketch is organized into four parts:

1. **Pin definitions & globals** — motor PWM channel, LED state, OLED object, `WebServer` on port 80.
2. **Web handlers** — `handleRoot()` serves an HTML page with inline CSS/JS; `handleLedOn()` / `handleLedOff()` toggle P26 and return a plain-text ack.
3. **OLED refresh** — `updateOLED()` redraws the IP, PWM bar, and LED state at ~10 Hz (throttled via `millis()` to avoid blocking the web server).
4. **Main loop** — `server.handleClient()` runs non-blockingly; `analogRead()` feeds `ledcWrite()` for smooth motor control.

> 💡 **Why `millis()` for OLED updates?** Calling `display.display()` (I2C write) on every loop iteration would starve `server.handleClient()` and make the web UI feel laggy. Throttling to 10 Hz keeps both responsive.

## Engineering Pitfalls

1. **Never drive a motor directly from GPIO.** The pin current limit is around 40mA; motors draw hundreds of mA and will damage the MCU.
2. **Flyback diode is not optional.** When the motor stops, its inductance produces a voltage spike that will kill the transistor. 1N4148 across the motor terminals absorbs it (use 1N4007 for higher-current motors).
3. **Base resistor value matters.** 220Ω for saturation; 1kΩ turns the transistor into a heater.
4. **Common ground.** External supply and ESP32 must share GND.
5. **Potentiometer wiring.** Only the wiper goes to the ADC pin. Connecting either end to 3.3V *and* the wiper to GND will short the rail through the pot.
6. **Audible motor whine.** If the motor emits a high-pitched sound, increase the LEDC PWM frequency (e.g. from 1 kHz to 20 kHz) in `ledcSetup()` to move it out of the audible range.
7. **Browser cache during web UI development.** Mobile browsers aggressively cache `fetch()` GET responses. If your LED toggle seems stuck, append a cache-buster query parameter (e.g. `/led/on?t=Date.now()`) or use `fetch(url, {cache: 'no-store'})`.

## Security Boundaries

This is a teaching project. The web UI has **no authentication** — anyone on the same network can toggle the LED. Only run this on a trusted LAN. For anything beyond a demo, add HTTP basic auth or a session token.

## Related Work

- **[OmniForge-Data-Annotation](https://github.com/Alexander390370/OmniForge-Data-Annotation)** — full-modal data annotation
- **[sd-forge-8gb-vram-setup](https://github.com/Alexander390370/sd-forge-8gb-vram-setup)** — Stable Diffusion on the same 8GB card
- **[esp32-edge-ai-security](https://github.com/Alexander390370/esp32-edge-ai-security)** — edge AI on the hardware side
- **[esp32-pwm-fan-controller](https://github.com/Alexander390370/esp32-pwm-fan-controller)** — hardware-side firmware

## License

Distributed under the MIT License. See [LICENSE](./LICENSE) for details.
