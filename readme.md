# FerroWave

Audio-reactive **ferrofluid** driver built on **ESP32-A1S (ES8388)**.  
Streams Bluetooth audio (A2DP) into the on-board codec, extracts a live envelope, and drives:
- a **coil + MOSFET** (PWM) for fluid motion, and
- a **WS2812 LED ring** (BitBang on GPIO21) for visuals.

> Goal: a clean, reproducible prototype you can flash, tune from the Serial Monitor, and extend.

---

## Table of Contents
- [Features](#features)
- [Hardware](#hardware)
- [Wiring (quick reference)](#wiring-quick-reference)
- [Firmware Setup (Arduino IDE)](#firmware-setup-arduino-ide)
- [Required Libraries](#required-libraries)
- [Build & Upload](#build--upload)
- [Serial Tuning Panel](#serial-tuning-panel)
- [How to Use](#how-to-use)
- [Troubleshooting](#troubleshooting)
- [Repository Layout](#repository-layout)
- [License](#license)

---

## Features
- **Bluetooth A2DP sink** → ES8388 line/headphone out (clean stereo).
- **Magnet control** via LEDC **PWM** with tunable frequency & duty window.
- **Audio envelope** drives the coil (slow LPF) and LEDs (fast LPF).
- **LED ring** reactive modes: Rainbow scroll, VU bar; master brightness.
- **Serial tuning panel** for live tweaks (freq, duty, sensitivity, filters, LED params) + status/telemetry.
- Designed to be **stable on ESP32 core 2.0.14** with NeoPixel **BitBang** to avoid RMT conflicts.

---

## Hardware
- **ESP32-A1S / ESP32-Audio-Kit** (with **ES8388** codec).
- **WS2812B LED ring** (24 px recommended).
- **Low-side MOSFET switch** for the **coil** (AOD4184 “trigger” module is fine).
- **Flyback diode** across the coil (e.g., **SS34 / 1N5819 / UF4007**).
- 5 V supply for LEDs & board; separate or well-filtered rail for the coil recommended.

**Pins used (default):**
- **LED ring DIN → GPIO 21** (NeoPixelBus BitBang)
- **MOSFET gate → GPIO 5** (LEDC channel 0)

> Avoid boot/strapping issues: ensure external circuitry doesn’t pull GPIO0/2/5/12/15 at reset.

---

## Wiring (quick reference)
5V Rail ──> ESP32-A1S 5V
└─> LED Ring 5V
GND ──> ESP32-A1S GND ─┬─> LED Ring GND
└─> MOSFET Source (S)

LED Ring DIN ──> GPIO 21 (through short wire; optional 330 Ω series)

Coil + ──> +V (coil rail: 5–12 V typical)
Coil − ──> MOSFET Drain (D)
Flyback diode: cathode to +V, anode to Coil − (at the coil)

ESP32 GPIO 5 ──> MOSFET Gate (G) (add 100 Ω gate R + 100 k pulldown to GND)


---

## Firmware Setup (Arduino IDE)
1. **Boards Manager** → install **ESP32 by Espressif** **`2.0.14`** (pinned).
2. **Select board**: any ESP32 dev board works for compile; upload to your A1S.
3. Partition: default is fine. PSRAM **off** unless you know you need it.

---

## Required Libraries
- **Arduino Audio Tools** — audio pipeline, streams  
  https://github.com/pschatzmann/arduino-audio-tools
- **Arduino Audio Driver** — ES8388 / AudioKitHAL codec drivers  
  https://github.com/pschatzmann/arduino-audio-driver
- **ESP32-A2DP** — Bluetooth A2DP sink/source  
  https://github.com/pschreibfaul1/ESP32-A2DP
- **NeoPixelBus by Makuna** — WS2812 LEDs (**use BitBang on GPIO21**)  
  https://github.com/Makuna/NeoPixelBus

> **Tip:** Remove/disable any old `audiokit` legacy library to avoid header/API conflicts.

### Install via Library Manager (GUI)
**Arduino IDE → Tools → Manage Libraries…**  
Search and install: `arduino-audio-tools`, `arduino-audio-driver`, `ESP32 A2DP`, `NeoPixelBus by Makuna`.

