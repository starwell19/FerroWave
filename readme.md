# FerroWave

Audio-reactive **ferrofluid** driver built on **ESP32-A1S (ES8388)**.  
Streams Bluetooth audio (A2DP) into the on-board codec, extracts live envelopes, and drives:

- a **coil + MOSFET trigger module** (PWM on GPIO 22) for ferrofluid motion  
- a **WS2812B LED ring** (GPIO 23) for synced visuals  

> Goal: a clean, reproducible prototype you can flash, tune from the Serial Monitor, and extend.

---

## Project Status (Extended Presets Edition)

This repo currently reflects the **“Extended Presets Edition”** firmware and the latest hardware layout.

**Key recent changes:**

- Switched to a **low-side MOSFET trigger module** (AOD4184-style “trigger” board):
  - Built-in flyback diode + status LED  
  - 3.3 V logic input, 5–12 V coil output  
  - Much more backer-friendly than raw MOSFET wiring
- Added a **USB-C PD trigger module**:
  - Use any common USB-C PD powerbank / charger  
  - Negotiates a stable 5–12 V rail (configurable)  
  - Lets us power **ESP32 + LEDs from 5 V**, and the **coil from a clean rail**
- Finalized **pinout** to avoid conflicts with the ES8388 codec, onboard keys and amp enable:
  - GPIO 22 → **MOSFET gate** (coil PWM)  
  - GPIO 23 → **WS2812B LED DIN**  
  - ES8388 on standard I²S pins (27 / 25 / 26 / 35 / 0)

This firmware is considered the **last known good base** for further work.

---

## Table of Contents

- [Features](#features)
- [Hardware](#hardware)
- [Wiring (quick reference)](#wiring-quick-reference)
- [Power (USB-C PD Trigger)](#power-usb-c-pd-trigger)
- [Pinout](#pinout)
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
- [Roadmap / What’s Next](#roadmap--whats-next)
- [Repository Layout](#repository-layout)
- [License](#license)

---

## Features

- **Bluetooth A2DP sink** → ES8388 line/headphone out (clean stereo to PAM8406 amp).
- **Magnet control** via LEDC **PWM** on GPIO 22 (channel 1) with:
  - adjustable frequency (`f<num>`)
  - tunable duty window (`d<num>`, `b<num>`).
- **Audio envelopes** (fast / slow / ultra-slow / peak) drive:
  - the **coil** (different motion behaviours per preset)
  - the **LED ring** (patterns and intensity).
- **8 magnet behaviour modes**:
  - SMOOTH, SPIKE, BOUNCE, CHAOS, PULSE, WAVE, TREMOLO, BREATH.
- **10 LED modes**:
  - rainbow, spectrum, VU, pulse, bass glow, fire, ocean, strobe, chase, sparkle.
- **Serial tuning panel**:
  - live control of PWM frequency, sensitivity, attack/release, duty limits, spike intensity  
  - LED brightness and mode selection  
  - test pulse and status printout.
- Designed to be **stable on ESP32 core 2.0.14**.

---

## Hardware

- **ESP32-A1S / ESP32-Audio-Kit v2.2** (with **ES8388** codec).
- **WS2812B LED ring** (24 LEDs recommended).
- **Low-side MOSFET trigger module**  
  (AOD4184 or similar “MOSFET trigger” module with built-in diode & LED).
- **Electromagnet** (5–12 V, mounted behind the ferrofluid bottle).
- **Flyback diode** (often integrated on the trigger module; otherwise add SS34 / 1N5819 / UF4007 across coil).
- **USB-C PD trigger module** for clean voltage from a PD powerbank or charger.
- 5 V rail for ESP32 + LEDs; separate or shared rail for coil depending on design.

---

## Wiring (quick reference)

5V Rail (from PD trigger) ──> ESP32-A1S 5V
                           └─> LED Ring 5V
GND ──> ESP32-A1S GND ──┬─> LED Ring GND
                        └─> MOSFET Source (S)

LED Ring DIN ──> GPIO 23 (through ~330 Ω recommended)

Coil +  ──> +V (coil rail: 5–12 V from PD trigger or dedicated supply)
Coil −  ──> MOSFET Drain (D)

Flyback diode:
  Cathode → +V (coil rail)
  Anode   → Coil − (at coil / MOSFET D)

ESP32 GPIO 22 ──> MOSFET Gate (G)
  Optional: 100 Ω gate resistor + 100 kΩ pulldown to GND.

The PD trigger module connects to a USB-C PD powerbank and negotiates the desired voltage (e.g. 9–12 V for the coil rail). You can then derive 5 V for the ESP32/LEDs (directly from PD if set to 5 V, or via a buck converter if running coil at higher voltage).

---

## Power (USB-C PD Trigger)

To keep things simple and portable:

- Use a **USB-C PD powerbank** or PD wall adapter.
- The **PD trigger** negotiates a fixed voltage (5–12 V).
- Recommended setups:
  - **Simple / low coil power:** PD trigger at 5 V → ESP32 + LEDs + coil all from 5 V.
  - **Higher coil power:** PD trigger at 9–12 V → coil on 9–12 V, step down to 5 V for ESP32 + LEDs.

Always respect the **current rating** of your source and watch coil temperature at high duty settings.

---

## Pinout

**ESP32-A1S v2.2 (Extended Presets Edition):**

- **GPIO 22** → Coil MOSFET gate (PWM, LEDC channel 1)
- **GPIO 23** → WS2812B LED ring DIN

**Audio / ES8388 (via AudioTools / AudioBoardStream):**

- **GPIO 27** → I²S BCK
- **GPIO 25** → I²S WS (LRCK)
- **GPIO 26** → I²S DATA_OUT (to DAC)
- **GPIO 35** → I²S DATA_IN (from ADC/mic)
- **GPIO 0**  → I²S MCLK

> Pins tied to onboard buttons / LEDs / amp enable (5, 18, 19, 21) are avoided to prevent conflicts.

---

## Firmware Setup (Arduino IDE)

1. **Boards Manager** → install **ESP32 by Espressif** `2.0.14` (recommended baseline).
2. **Select Board:**  
   Use **“ESP32 Dev Module”** (works for compile/upload to A1S).
3. **Partition Scheme:** default is fine.  
   PSRAM can stay **disabled** unless you know you need it.

---

## Required Libraries

Install these via **Arduino Library Manager**:

- **Arduino Audio Tools** — audio pipeline, streams  
  https://github.com/pschatzmann/arduino-audio-tools
- **Arduino Audio Driver** — ES8388 / AudioKitHAL codec drivers  
  https://github.com/pschatzmann/arduino-audio-driver
- **ESP32-A2DP** — Bluetooth A2DP sink/source  
  https://github.com/pschreibfaul1/ESP32-A2DP
- **Adafruit NeoPixel** — WS2812 LEDs  
  https://github.com/adafruit/Adafruit_NeoPixel

> Tip: Remove/disable any old `audiokit` legacy libraries that conflict with `AudioTools` + `AudioDriver`.

---

## Build & Upload

1. Clone this repo or copy the main `.ino` into your Arduino sketch folder (e.g. `FerroWave_ExtendedPresets.ino`).
2. Select the correct **COM port** for your ESP32-A1S.
3. Press **Upload**.
4. Open **Serial Monitor** at **115200 baud**.
5. Pair your phone/PC via Bluetooth (device name e.g. `FerroWave_BackCoil`) and start playing music.

---

## Serial Tuning Panel

All runtime control is done via the **Serial Monitor (115200 baud)**.

You can:

- switch between **8 magnet modes** (`1`–`8`)
- switch between **10 LED modes** (`c1`–`c10`)
- tweak **tuning parameters** (`f`, `s`, `a`, `r`, `d`, `b`, `p`, `l`)
- run **test pulses** and view **status readouts**

### Magnet Modes (1–8)

Command | Mode     | Description
-------:|----------|----------------------------------------------------------
1       | SMOOTH   | Gentle flowing waves, slow response
2       | SPIKE    | Sharp transients, emphasizes beats/spikes
3       | BOUNCE   | Rhythmic pumping, follows groove
4       | CHAOS    | Aggressive, unpredictable, pushes power envelope
5       | PULSE    | Distinct on/off pulses based on beat detection
6       | WAVE     | Slow building surges, energy accumulation
7       | TREMOLO  | Rapid flutter / vibrato-style motion
8       | BREATH   | Slow meditative breathing, ultra-smooth “breathing”

---

### LED Modes (c1–c10)

Use `c<num>` (e.g. `c3`, `c8`) to select LED mode.

Command | Mode      | Description
-------:|-----------|----------------------------------------------
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

Pattern: `letter<number>`  
Examples: `f4`, `s120`, `a80`, `l180` …

Command | Parameter       | Range       | Default | Description
-------:|-----------------|------------|---------|-----------------------------------------------
f<num>  | PWM Frequency   | 1–5000 Hz  | f4      | How fast the magnet pulses
s<num>  | Sensitivity     | 0–200 %    | s100    | How strongly audio drives motion
a<num>  | Attack Speed    | 0–100      | a60     | How fast magnet responds to peaks
r<num>  | Release Speed   | 0–100      | r30     | How fast magnet relaxes / decays
d<num>  | Max Duty        | 10–100 %   | d80     | Maximum power ceiling (coil safety)
b<num>  | Base Duty       | 0–50 %     | b15     | Minimum always-on power (baseline field)
p<num>  | Spike Intensity | 0–100      | p50     | Extra boost for SPIKE / transient-heavy modes
l<num>  | LED Brightness  | 0–255      | l100    | LED strip brightness

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

You can treat these as “scenes” by typing each command sequence into the Serial Monitor.

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

### Beat Sync (tight pulses)

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
l255    # max LED brightness

---

## Quick Tips

**For more spikes / deformation:**

- Higher **attack**: a80–a100
- Higher **spike intensity**: p70–p100
- Use **SPIKE** (2) or **CHAOS** (4)
- Lower PWM frequency: f4–f10

**For smooth, flowing behaviour:**

- Lower attack/release: a30, r20
- Use **SMOOTH** (1) or **BREATH** (8)
- Higher PWM frequency: f500–f1000

**For rhythmic bouncing:**

- Use **BOUNCE** (3) or **PULSE** (5)
- Similar attack/release (e.g. a70, r70)
- PWM around f10–f50

**Power management (coil safety):**

- Start with d60–d80
- Only push to d90–d100 if PSU & thermals can handle it
- Use b10–b20 for a gentle constant field

---

## Parameter Cheat Sheet

**PWM Frequency (f<num>):**

- f2–f10: slow breathing, visible pulsing  
- f50–f500: smooth mid-range control  
- f1000–f2000: very fast, nearly continuous force

**Sensitivity (s<num>):**

- s50: subtle, needs louder music  
- s100: default  
- s150+: aggressive, reacts to small changes  

**Attack / Release (a<num>, r<num>):**

- Low (10–40): smooth, sluggish  
- Mid (50–70): musical, responsive  
- High (80–100): snappy, percussive  

---

## How to Use

1. Flash the firmware and open Serial Monitor at 115200 baud.
2. Pair your phone/PC over Bluetooth (`FerroWave_BackCoil` or similar).
3. Start playing music.
4. Select:
   - a magnet mode (1–8), and
   - an LED mode (c1–c10).
5. Optionally apply one of the preset combos above.
6. Watch ferrofluid + LEDs react in real time.

---

## Troubleshooting

- **No Bluetooth audio**
  - Make sure `ESP32-A2DP` is installed and the sketch compiled cleanly.
  - Verify you’re paired with the correct BT name.
  - Try power-cycling after upload.

- **LED ring is dark**
  - Check DIN on GPIO 23 and that GND is shared.
  - Verify `Adafruit NeoPixel` is installed.
  - Try `l255` + `c1` to force a visible pattern at full brightness.

- **Coil always on / always off**
  - Check MOSFET wiring: S→GND, D→coil−, coil+ → +V.
  - Confirm flyback diode orientation (if external).
  - Use `t` to trigger a 2s test pulse and see if the MOSFET LED reacts.

- **Random resets / glitches**
  - Use a solid PSU or PD powerbank (2–3 A).  
  - Consider separating the coil rail or adding bulk caps near the coil and board.

---

## Roadmap / What’s Next

Planned next steps for this project:

- **3D-printable enclosure**  
  - Adapt enclosure design to:
    - ESP32-A1S board  
    - MOSFET trigger module  
    - USB-C PD trigger  
    - Magnet + bottle + LED ring stack  
  - Goals: easy assembly, good cooling, nice desk presence.

- **GitHub polish**
  - Finalize wiring diagrams and BOM.
  - Provide photos/renders of a clean, non-breadboard build.
  - Possibly split into:
    - **User firmware** (simple presets)
    - **Dev firmware** (full serial tuning playground).

- **Backer / kit variants (for future)**
  - Define what a “kit” vs. “DIY files only” vs. “assembled unit” entails.

---

## Repository Layout

- `FerroWave_ExtendedPresets.ino` — main sketch (magnet + LED presets, serial panel).
- `README.md` — this file.
- `hardware/` — schematics, wiring, PD trigger + MOSFET details (planned).
- `enclosure/` — 3D models / STLs for the case and bottle mount (planned).

---

## License

TBD (likely MIT or similar permissive license).
