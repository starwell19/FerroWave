Open Serial Monitor (115200). Type these single-key commands:

General

h → help / key map

? → print one-line status

p → toggle periodic status (every ~3 s)

Z → restore defaults

Frequency (coil PWM)

+ / - → freq up/down

≤ 8 Hz: 0.25 Hz steps (fine)

8 Hz: 1.00 Hz steps (coarse)

Typical ferro “sweet spot”: 4–6 Hz

Magnet dynamics

m → toggle magnet enable

[ / ] → dutyMin −/+ 2% (lower bound)

{ / } → dutyMax −/+ 2% (upper bound)

b / B → baseDutyOffset −/+ 1% (−20…+20%, applied before clamping)

Sensitivity & smoothing

e / E → sensitivity −/+ 0.10 (0.20…3.00)

u / U → LPFslow −/+ 0.005 (0.005…0.200) → magnet smoothness

y / Y → LPFfast −/+ 0.05 (0.05…0.80) → LED snappiness

LEDs

l → LEDs on/off

0 / 1 / 2 → LED OFF / RAINBOW / VU

g / G → LED brightness −/+ 0.05 (0.05…1.00)

r / R → rainbow base speed −/+ 0.001

t / T → rainbow gain speed (reactivity) −/+ 0.005

VU meter

v / V → VU decay −/+ 0.01 (0.01…0.20)

Use ? to capture a full status line of your chosen settings for notes/presets.


You likely have mixed/duplicate libraries. Remove any legacy audiokit folder and keep only the four libraries listed above, with ESP32 core 2.0.14.
