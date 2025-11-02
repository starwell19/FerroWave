---
title: FerroWave
---

# FerroWave

Audio-reactive ferrofluid driver with ESP32-A1S (ES8388), Bluetooth A2DP, and WS2812 LED ring.

## Quick start
1. Boards Manager → **ESP32 by Espressif 2.0.14**
2. Libs: **arduino-audio-tools**, **arduino-audio-driver**, **ESP32-A2DP**, **NeoPixelBus**
3. Open `firmware/FerroWave.ino`, upload, open Serial 115200, type **`h`**.

## Serial commands (highlights)
`+/-` freq • `m` magnet • `l` LEDs • `0/1/2` modes • `[` `]` dutyMin • `{` `}` dutyMax • `b/B` base duty • `e/E` sens • `u/U` LPFslow • `y/Y` LPFfast • `g/G` brightness • `r/R` base speed • `t/T` gain speed • `v/V` decay • `?` status • `p` periodic • `Z` defaults
