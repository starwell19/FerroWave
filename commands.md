# FerroWave Serial Commands Reference

Complete command reference for controlling FerroWave via Serial Monitor (115200 baud).

> **Note:** Physical buttons provide quick access to presets. Serial commands are for advanced tuning and diagnostics.

---

## Quick Reference

| Command | Function | Example |
|---------|----------|---------|
| `1-8` | Select magnet mode | `3` |
| `c1-c10` | Select LED mode | `c7` |
| `f<num>` | Set PWM frequency | `f500` |
| `s<num>` | Set sensitivity | `s120` |
| `a<num>` | Set attack speed | `a80` |
| `r<num>` | Set release speed | `r40` |
| `d<num>` | Set max duty | `d85` |
| `b<num>` | Set base duty | `b20` |
| `p<num>` | Set spike intensity | `p70` |
| `l<num>` | Set LED brightness | `l150` |
| `v<num>` | Set volume | `v75` |
| `aux` | Switch to AUX input | `aux` |
| `bt` | Switch to Bluetooth | `bt` |
| `?` | Show current settings | `?` |
| `t` | Test pulse | `t` |
| `m` | List magnet modes | `m` |
| `n` | List LED modes | `n` |

---

## Magnet Mode Selection

**Syntax:** Single digit `1-8`

| Command | Mode | Description | Characteristics |
|---------|------|-------------|-----------------|
| `1` | SMOOTH | Gentle flowing waves | Slow, graceful, relaxed |
| `2` | SPIKE | Sharp transients | Emphasizes beats, spiky |
| `3` | BOUNCE | Rhythmic pumping | Follows groove, bouncy |
| `4` | CHAOS | Aggressive random | Unpredictable, maximum power |
| `5` | PULSE | Beat-synced pulses | Distinct on/off, percussive |
| `6` | WAVE | Building surges | Slow accumulation, dramatic |
| `7` | TREMOLO | Rapid flutter | Vibrato effect, rapid |
| `8` | BREATH | Meditative breathing | Very slow, calming |

**Example:**
```
> 2
Mode -> SPIKE
```

---

## LED Mode Selection

**Syntax:** `c<number>` where number is 1-10

| Command | Mode | Description | Effect Type |
|---------|------|-------------|-------------|
| `c1` | Rainbow | Spinning rainbow wheel | Color cycle |
| `c2` | Spectrum | Bar graph analyzer | Audio reactive |
| `c3` | Pulse | Blue breathing effect | Single color pulse |
| `c4` | VU-Meter | Center-split meter | Audio level |
| `c5` | Bass Glow | Red bass intensity | Audio reactive |
| `c6` | Fire | Flickering flames | Animated |
| `c7` | Ocean | Blue-green waves | Animated |
| `c8` | Strobe | White flash on peaks | Audio reactive |
| `c9` | Chase | Three colored dots | Animated |
| `c10` | Sparkle | Random twinkling | Animated |

**Example:**
```
> c7
LED mode -> Ocean
```

---

## Tuning Parameters

### PWM Frequency (`f`)

**Syntax:** `f<number>`  
**Range:** 1-5000 Hz  
**Default:** 4 Hz

Controls how fast the electromagnet pulses.

**Recommended ranges:**
- `f2-f10`: Slow, visible pulsing (dramatic movement)
- `f50-f500`: Mid-range smooth control
- `f1000-f2000`: Very fast, near-continuous force

**Examples:**
```
> f4
PWM frequency -> 4.0 Hz

> f500
PWM frequency -> 500.0 Hz

> f1000
PWM frequency -> 1000.0 Hz
```

---

### Sensitivity (`s`)

**Syntax:** `s<number>`  
**Range:** 0-200%  
**Default:** 100%

Controls how strongly audio input affects the magnet.

**Guide:**
- `s50`: Subtle, requires loud music
- `s100`: Default, balanced
- `s150`: Sensitive, reacts to small changes
- `s200`: Maximum sensitivity

**Examples:**
```
> s120
Sensitivity -> 120%

> s80
Sensitivity -> 80%
```

---

### Attack Speed (`a`)

**Syntax:** `a<number>`  
**Range:** 0-100  
**Default:** 60

How fast the magnet responds to increasing audio levels.

**Guide:**
- `a10-a40`: Slow, smooth response
- `a50-a70`: Musical, natural
- `a80-a100`: Snappy, percussive

**Examples:**
```
> a85
Attack speed -> 85/100

> a30
Attack speed -> 30/100
```

---

### Release Speed (`r`)

**Syntax:** `r<number>`  
**Range:** 0-100  
**Default:** 30

How fast the magnet relaxes when audio decreases.

**Guide:**
- `r10-r30`: Slow decay, sustained
- `r40-r60`: Moderate release
- `r70-r100`: Fast drop, tight

**Examples:**
```
> r50
Release speed -> 50/100

> r20
Release speed -> 20/100
```

---

### Max Duty (`d`)

**Syntax:** `d<number>`  
**Range:** 10-100%  
**Default:** 80%

Maximum power ceiling for electromagnet (safety limit).

**Guide:**
- `d60-d80`: Safe, conservative
- `d85-d95`: Higher power (watch temperature)
- `d100`: Maximum (only with adequate cooling)

⚠️ **Warning:** Higher values generate more heat. Monitor coil temperature!

**Examples:**
```
> d75
Max duty -> 75%

> d90
Max duty -> 90%
```

---

### Base Duty (`b`)

**Syntax:** `b<number>`  
**Range:** 0-50%  
**Default:** 15%

Minimum always-on power (baseline magnetic field).

**Guide:**
- `b0`: Off when silent (dynamic range)
- `b10-b20`: Gentle constant field
- `b30-b50`: Strong baseline (less dynamic)

**Examples:**
```
> b20
Base duty -> 20%

> b5
Base duty -> 5%
```

---

### Spike Intensity (`p`)

**Syntax:** `p<number>`  
**Range:** 0-100  
**Default:** 50

Extra boost for SPIKE and transient-heavy modes.

**Guide:**
- `p30-p50`: Moderate emphasis
- `p60-p80`: Strong spikes
- `p90-p100`: Maximum aggression

**Examples:**
```
> p75
Spike intensity -> 75/100

> p40
Spike intensity -> 40/100
```

---

### LED Brightness (`l`)

**Syntax:** `l<number>`  
**Range:** 0-255  
**Default:** 100

LED ring brightness level.

**Guide:**
- `l50`: Dim (power saving)
- `l100`: Default
- `l150`: Bright
- `l255`: Maximum

**Examples:**
```
> l180
LED brightness -> 180/255

> l60
LED brightness -> 60/255
```

---

### Volume (`v`)

**Syntax:** `v<number>`  
**Range:** 0-100%  
**Default:** 80%

Audio output volume level.

**Examples:**
```
> v75
Volume -> 75%

> v90
Volume -> 90%
```

---

## Audio Source Control

### Switch to AUX Input

**Syntax:** `aux`

Manually switch to 3.5mm AUX line input.

```
> aux
Switching to AUX input...
AUX input active
```

---

### Switch to Bluetooth

**Syntax:** `bt`

Manually switch to Bluetooth A2DP input.

```
> bt
Switching to Bluetooth...
Bluetooth active
```

> **Note:** AUX input auto-switches when cable is detected. These commands allow manual override.

---

## Utility Commands

### Show Current Settings (`?`)

**Syntax:** `?`

Displays all current parameter values and modes.

```
> ?

========== CURRENT SETTINGS ==========
Audio Source:    Bluetooth
Magnet Mode:     SPIKE
PWM Frequency:   4.0 Hz
Sensitivity:     120%
Attack Speed:    85/100
Release Speed:   40/100
Max Duty:        80%
Base Duty:       15%
Spike Intensity: 75/100
Volume:          80%
EQ Preset:       Bass Boost
Bass EQ:         +6
Treble EQ:       -2
LED Brightness:  100/255
LED Mode:        Fire
======================================
```

---

### Test Pulse (`t`)

**Syntax:** `t`

Triggers a 2-second full-power test pulse with LED flash.

Useful for:
- Testing electromagnet wiring
- Verifying MOSFET functionality
- Checking power supply capacity

```
> t
TEST PULSE: 2 seconds at max...
Test complete.
```

---

### List Magnet Modes (`m`)

**Syntax:** `m`

Displays all available magnet modes.

```
> m

========== MAGNET MODES ==========
1 = SMOOTH  - Gentle flowing waves
2 = SPIKE   - Sharp transients, spiky
3 = BOUNCE  - Rhythmic pumping
4 = CHAOS   - Aggressive, unpredictable
5 = PULSE   - Distinct on/off beats
6 = WAVE    - Slow building surges
7 = TREMOLO - Rapid flutter effect
8 = BREATH  - Slow meditative breathing
==================================
```

---

### List LED Modes (`n`)

**Syntax:** `n`

Displays all available LED effects.

```
> n

========== LED MODES ==========
c1  = Rainbow spin
c2  = Spectrum analyzer
c3  = Pulse (blue)
c4  = VU-Meter
c5  = Bass glow (red)
c6  = Fire effect
c7  = Ocean waves
c8  = Strobe flash
c9  = Color chase
c10 = Sparkle
===============================
```

---

## Preset Recipes

### Quick-Start Presets

Copy and paste these command sequences for instant presets:

#### 🎉 Party Mode
```
2
c8
f4
s150
a90
r40
p80
l200
```

#### 🌊 Chill Mode
```
8
c7
f2
s60
a30
r20
l100
```

#### 💃 Dance Mode
```
3
c2
f10
s120
a80
r60
l150
```

#### 🔥 Fire Show
```
4
c6
f500
s140
a85
d90
l180
```

---

## Tips & Best Practices

### For Experimentation
1. Start with `?` to see current settings
2. Change one parameter at a time
3. Use `t` to verify electromagnet response
4. Watch Serial output for real-time feedback

### For Performance
1. Set your desired mode (`1-8`)
2. Set LED effect (`c1-c10`)
3. Fine-tune sensitivity (`s`) first
4. Adjust attack/release (`a`, `r`) for feel
5. Set PWM frequency (`f`) for desired motion style

### For Safety
1. Always start with `d80` or lower
2. Monitor coil temperature when increasing `d`
3. Use `b` to set a safe baseline
4. Test with `t` before live performance

---

## Troubleshooting Commands

### Magnet not responding?
```
> t          # Test pulse
> d90        # Increase max duty
> s150       # Increase sensitivity
> b20        # Add baseline field
```

### Too aggressive?
```
> s80        # Reduce sensitivity
> d70        # Lower power ceiling
> a40        # Slow attack
> r30        # Slow release
```

### LEDs too dim?
```
> l200       # Increase brightness
> c1         # Try rainbow (brightest)
```

### Audio source issues?
```
> aux        # Switch to AUX
> bt         # Switch to Bluetooth
> v80        # Adjust volume
```

---

**For physical button controls, see the main README.md**

---

*Part of the [FerroWave](https://github.com/matoslav/FerroWave) project by Makarov87*
