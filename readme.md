# FerroWave

Audio-reactive **ferrofluid** driver built on **ESP32-A1S (ES8388)**.  
Streams Bluetooth audio (A2DP) into the on-board codec, extracts live envelopes, and drives:

- a **coil + MOSFET** (PWM on GPIO 22) for ferrofluid motion  
- a **WS2812B LED ring** (GPIO 23) for synced visuals  

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
  - [Magnet Modes (1–8)](#magnet-modes-1-8)
  - [LED Modes (c1–c10)](#led-modes-c1c10)
  - [Tuning Parameters](#tuning-parameters)
  - [Utility Commands](#utility-commands)
  - [Recommended Presets](#recommended-presets)
  - [Quick Tips](#quick-tips)
  - [Parameter Cheat Sheet](#parameter-cheat-sheet)
- [How to Use](#how-to-use)
- [Troubleshooting](#troubleshooting)
- [Repository Layout](#repository-layout)
- [License](#license)

---

## Features

- **Bluetooth A2DP sink** → ES8388 line/headphone out (clean stereo).
- **Magnet control** via LEDC **PWM** with tunable frequency & duty window.
- **Audio envelopes** (fast / slow / ultra-slow / peak) drive:
  - the **coil** (different motion behaviours per preset)
  - the **LED ring** (patterns + intensity).
- **8 magnet behaviour modes** (SMOOTH, SPIKE, BOUNCE, CHAOS, PULSE, WAVE, TREMOLO, BREATH).
- **10 LED modes** (rainbow, spectrum, fire, ocean, strobe, sparkle, etc.).
- **Serial tuning panel**:
  - live tweaks for PWM freq, envelope speeds, sensitivity, duty range, spike intensity
  - LED brightness and LED mode selection
  - debug/log output.
- Designed to be **stable on ESP32 core 2.0.14**.

---

## Hardware

- **ESP32-A1S / ESP32-Audio-Kit** (with **ES8388** codec).
- **WS2812B LED ring** (24 px recommended).
- **Low-side MOSFET switch** for the **coil** (AOD4184 “trigger” module is fine).
- **Flyback diode** across the coil (e.g., **SS34 / 1N5819 / UF4007**).
- 5 V supply for LEDs & board; separate or well-filtered rail for the coil recommended.

**Pins used (Extended Presets Edition):**

- **MOSFET gate → GPIO 22** (LEDC PWM channel 1)
- **LED ring DIN → GPIO 23** (Adafruit_NeoPixel, NEO_GRB @ 800 kHz)

> Reminder: avoid messing with boot/strapping pins during reset (GPIO0/2/5/12/15).

---

## Wiring (quick reference)

5V Rail ──> ESP32-A1S 5V
          └─> LED Ring 5V
GND   ──> ESP32-A1S GND ─┬─> LED Ring GND
                         └─> MOSFET Source (S)

LED Ring DIN ──> GPIO 23 (through 330 Ω resistor recommended)

Coil +  ──> +V (coil rail: 5–12 V typical)
Coil −  ──> MOSFET Drain (D)

Flyback diode:
  Cathode → +V (coil rail)
  Anode   → Coil − (at the coil / MOSFET D)

ESP32 GPIO 22 ──> MOSFET Gate (G)
  Optional: ~100 Ω gate resistor + 100 kΩ pulldown to GND.

---

## Firmware Setup (Arduino IDE)

1. Boards Manager → install ESP32 by Espressif `2.0.14` (pinned).
2. Select board:
   - Any “ESP32 Dev Module” works for compile; upload directly to your A1S.
3. Partition: default is fine. PSRAM off unless you know you need it.

---

## Required Libraries

- Arduino Audio Tools — audio pipeline, streams  
  https://github.com/pschatzmann/arduino-audio-tools
- Arduino Audio Driver — ES8388 / AudioKitHAL codec drivers  
  https://github.com/pschatzmann/arduino-audio-driver
- ESP32-A2DP — Bluetooth A2DP sink/source  
  https://github.com/pschreibfaul1/ESP32-A2DP
- Adafruit NeoPixel — WS2812 LEDs  
  https://github.com/adafruit/Adafruit_NeoPixel

Tip: Remove/disable any old `audiokit` legacy library to avoid header/API conflicts.

### Install via Library Manager (GUI)

Arduino IDE → Tools → Manage Libraries…  
Search and install:  
`arduino-audio-tools`, `arduino-audio-driver`, `ESP32 A2DP`, `Adafruit NeoPixel`.

---

## Build & Upload

1. Clone this repository or drop the .ino into your Arduino sketch folder.
2. Select the correct port for your ESP32-A1S.
3. Hit Upload.
4. Open Serial Monitor at 115200 baud to see debug + command interface.
5. Pair your phone/PC with the board’s BT name (e.g. `FerroWave_BackCoil`) and play music.

---

## Serial Tuning Panel

All interaction happens via the Serial Monitor at 115200 baud.  
You can:

- switch between 8 magnet modes (1–8)
- switch between 10 LED modes (c1–c10)
- tweak tuning parameters (f, s, a, r, d, b, p, l)
- run test pulses and print current settings.

### Magnet Modes (1–8)

Command | Mode     | Description
-------:|----------|------------
1       | SMOOTH   | Gentle flowing waves, slow response
2       | SPIKE    | Sharp transients, emphasizes beats/spikes
3       | BOUNCE   | Rhythmic pumping, follows groove
4       | CHAOS    | Aggressive, unpredictable, pushes power envelope
5       | PULSE    | Distinct on/off pulses based on beat detection
6       | WAVE     | Slow building surges, energy accumulation
7       | TREMOLO  | Rapid flutter / vibrato-style motion
8       | BREATH   | Slow meditative breathing, ultra-smooth movement

---

### LED Modes (c1–c10)

Use c<num> (e.g. c3, c8) to select LED mode.

Command | Mode      | Description
-------:|-----------|------------
c1      | Rainbow   | Classic spinning rainbow wheel
c2      | Spectrum  | Bar graph analyzer (bottom-up)
c3      | Pulse     | Single blue color breathing
c4      | VU-Meter  | Center-split bar / VU-style meter
c5      | Bass Glow | Red glow that intensifies with bass / env
c6      | Fire      | Flickering fire/flame effect
c7      | Ocean     | Blue-green waves (calming)
c8      | Strobe    | White flash on loud peaks
c9      | Chase     | Three colored dots chasing around the ring
c10     | Sparkle   | Random sparkles / twinkling points

---

### Tuning Parameters

All parameter commands follow the pattern: letter<number>  
Examples: f4, s120, a80, l180 …

Command  | Parameter       | Range       | Default | Description
--------:|-----------------|------------|---------|------------
f<num>   | PWM Frequency   | 1–5000 Hz  | f4      | How fast the magnet pulses
s<num>   | Sensitivity     | 0–200 %    | s100    | Audio gain / how strongly audio drives motion
a<num>   | Attack Speed    | 0–100      | a60     | How fast magnet responds to new audio peaks
r<num>   | Release Speed   | 0–100      | r30     | How fast magnet relaxes / decays
d<num>   | Max Duty        | 10–100 %   | d80     | Maximum power ceiling
b<num>   | Base Duty       | 0–50 %     | b15     | Minimum always-on power
p<num>   | Spike Intensity | 0–100      | p50     | Extra boost for SPIKE / transient modes
l<num>   | LED Brightness  | 0–255      | l100    | LED strip brightness

---

### Utility Commands

Command | Function
-------:|---------
?       | Show all current settings
m       | List all magnet modes
n       | List all LED modes
i       | Show detailed parameter info / explanations
t       | Test pulse (2 sec full power + LED flash)

---

## Recommended Presets

You can treat these as “scenes” or “profiles” by just typing the sequence of commands into the Serial Monitor.

### Spike Party (aggressive spikes)

2       # SPIKE mode
c8      # Strobe LEDs
f4      # slow visible pulsing
s150    # very sensitive
a90     # fast attack
r40     # moderate release
p80     # strong spikes

### Chill Waves (relaxing)

8       # BREATH mode
c7      # Ocean LEDs
f2      # very slow pulse
s60     # subtle
a30     # slow attack
r20     # slow release

### Dance Floor (pumping bass)

3       # BOUNCE mode
c2      # Spectrum LEDs
f10     # visible bounce
s120    # sensitive
a80     # snappy
r60     # quick release

### Fire Show (dramatic)

4       # CHAOS mode
c6      # Fire LEDs
f500    # smooth high-frequency PWM
s140    # intense sensitivity
a85     # fast response
d90     # allow higher power (watch thermals)

### Beat Sync (tight sync)

5       # PULSE mode
c4      # VU-Meter LEDs
f1000   # fast PWM
a95     # near-instant attack
r85     # quick drop
p60     # medium pulse strength

### Psychedelic (trippy)

7       # TREMOLO mode
c1      # Rainbow LEDs
f50     # mid-range flutter
s130    # sensitive

### Ambient (slow building)

6       # WAVE mode
c5      # Bass Glow LEDs
f4      # slow
s80     # moderate sensitivity
a40     # slow build
r30     # sustain

### Strobe Party (intense)

2       # SPIKE mode
c8      # Strobe LEDs
f1000   # smooth PWM
s180    # extreme sensitivity
a100    # max attack
p100    # max spikes
l255    # max LED brightness (full blast)

---

## Quick Tips

For more spikes / deformation:

- Use higher attack speed: a80–a100
- Higher spike intensity: p70–p100
- Use SPIKE (2) or CHAOS (4) modes
- Lower PWM frequency: f4–f10

For smooth, flowing behaviour:

- Lower attack/release speeds: a30, r20
- Use SMOOTH (1) or BREATH (8)
- Higher PWM frequency: f500–f1000

For rhythmic bouncing:

- Use BOUNCE (3) or PULSE (5)
- Keep attack/release similar (e.g. a70, r70)
- PWM around f10–f50

Power management (coil safety):

- Start with d60–d80 (Max Duty)
- Only go to d90–d100 if your PSU & thermals can handle it
- Use b10–b20 for a constant subtle field baseline

---

## Parameter Cheat Sheet

PWM Frequency (f<num>):

- f2–f10: slow breathing, visible pulsing
- f50–f500: smooth mid-range control
- f1000–f2000: very fast, smoother physical response (less obvious pulsing)

Sensitivity (s<num>):

- s50: subtle / needs louder music
- s100: normal / default
- s150+: aggressive, reacts heavily even to small changes

Attack / Release (a<num>, r<num>):

- Low (10–40): slow, smooth transitions
- Mid (50–70): balanced / musical
- High (80–100): snappy, instantly reacts

---

## How to Use

1. Flash firmware and open Serial Monitor at 115200.
2. Pair via Bluetooth (FerroWave_BackCoil) and start playing music.
3. Pick a magnet mode (1–8) and LED mode (c1–c10).
4. Optionally apply a preset macro from above or tune parameters manually.
5. Watch the ferrofluid and LEDs respond in real time.

---

## Troubleshooting

- No Bluetooth audio  
  - Check A2DP library installed correctly.  
  - Make sure you’re paired to the correct BT name.  
  - Try power-cycling the board after upload.

- LED strip not lighting  
  - Confirm DIN → GPIO 23 and common GND.  
  - Check Adafruit NeoPixel is installed and working.  
  - Try l255 to max brightness and c1 to force a visible pattern.

- Coil always on or always off  
  - Verify MOSFET wiring (S to GND, D to coil−, coil+ to +V).  
  - Check flyback diode polarity.  
  - Use t to run a 2-second test pulse and see if the MOSFET LED reacts.

- Glitchy behaviour / reboots  
  - Use a solid 5 V PSU (2–3 A recommended).  
  - Separate coil supply if necessary or add decoupling capacitors.

---

## Repository Layout

- FerroWave_ExtendedPresets.ino — main sketch (magnet + LED presets, serial panel).
- README.md — this documentation.
- (future) /hardware — schematics, wiring diagrams.
- (future) /enclosures — 3D printable parts, mounting for coil, bottle, and ring.

---

## License

TBD.
