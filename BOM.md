# FerroWave Bill of Materials (BOM)

Complete list of parts needed to build FerroWave.

**Project**: FerroWave - Ferrofluid Visualizer  
**Author**: Makarov87  
**Crowdfunded by**: MakerWorld Community  
**Version**: 2.0 (Battery-Powered Edition)

---

## Quick Summary

| Category | Estimated Cost | Notes |
|----------|---------------|-------|
| Electronics | $40-60 USD | Main components |
| Power System | $15-25 USD | Batteries and holder |
| Hardware/Misc | $10-20 USD | Wires, resistors, connectors |
| **Total** | **$65-105 USD** | Excluding ferrofluid & display |

*Prices approximate, may vary by region and supplier*

---

## Essential Components

### Microcontroller & Audio

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| ESP32-A1S Audio Kit v2.2 | With ES8388 codec | 1 | $15-25 | AliExpress, Amazon | Must be v2.2 or compatible |
| Micro USB Cable | Data capable | 1 | $3-5 | Anywhere | For programming |

**ESP32-A1S Sources:**
- AliExpress: Search "ESP32-A1S" or "ESP32 Audio Kit"
- Amazon: "ESP32 Audio Development Board"
- Local electronics suppliers

**Important:** Verify it includes:
- ES8388 audio codec
- 6 onboard buttons
- Micro USB port
- Pin headers (or ability to solder them)

---

### Power System

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| 18650 Battery Holder | 4-cell with 5V USB-A output | 1 | $8-12 | AliExpress, Amazon | Must have built-in 5V regulation |
| 18650 Li-ion Batteries | 2500-3500mAh, protected | 4 | $12-20 | Battery stores | Use quality cells only |
| USB-A to Breadboard Cable | Standard or cut cable | 1 | $2-3 | Anywhere | Connects battery to power module |
| Breadboard Power Module | Dual rail, 5V, with switch | 1 | $2-4 | AliExpress, Amazon | MB102 or similar |

**Battery Recommendations:**
- Samsung 25R (2500mAh, 20A)
- LG HG2 (3000mAh, 20A)
- Sony VTC6 (3000mAh, 15A)
- Panasonic NCR18650B (3400mAh, 6.8A)

⚠️ **Safety:** Always use protected cells from reputable brands. Avoid cheap unbranded batteries.

**18650 Holder Requirements:**
- Built-in boost converter to 5V
- USB-A output port (5V)
- Micro-USB charging port
- Protection circuitry
- Output current: 3A minimum, 5A recommended
- Wake button to activate output

---

### Electromagnet & Driver

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| MOSFET Trigger Module | AOD4184 or similar, 5V logic | 1 | $2-4 | AliExpress, Amazon | Must have flyback diode |
| Electromagnet | 5V, 1-2A, solenoid type | 1 | $5-10 | AliExpress, Amazon | 20mm-30mm diameter recommended |

**MOSFET Module Search Terms:**
- "MOSFET trigger module"
- "AOD4184 module"
- "MOSFET switch module 5V"
- "Low-side MOSFET driver"

**Required Features:**
- 3.3V/5V logic input
- Built-in flyback diode
- Status LED indicator
- 5A+ current handling

**Electromagnet Options:**

| Type | Specs | Best For | Price |
|------|-------|----------|-------|
| Small | 5V/1A, 20mm | Desktop display | $5-7 |
| Medium | 5V/1.5A, 25mm | Standard build | $7-10 |
| Large | 5V/2A, 30mm | Maximum effect | $10-15 |

---

### LED System

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| WS2812B LED Ring | 24 LEDs, 5V | 1 | $5-10 | AliExpress, Amazon | 12 or 16 LED also works |
| Jumper Wires | Female-Female, 10cm | 5-10 | $2-3 | Electronics store | For connections |

**LED Ring Options:**

| Size | LED Count | Diameter | Best For | Price |
|------|-----------|----------|----------|-------|
| Small | 12 LEDs | 37mm | Compact builds | $3-5 |
| Medium | 16 LEDs | 44mm | Standard | $4-6 |
| **Recommended** | **24 LEDs** | **66mm** | **Best effect** | **$6-10** |
| Large | 32 LEDs | 82mm | Big displays | $8-12 |

**LED Strip Alternative:**
- WS2812B strip, 5V, 60 LEDs/m
- Cut to desired length
- More flexible mounting
- $8-12 per meter

---

### Electronic Components

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| Resistor | 330Ω, 1/4W | 1 | $0.10 | Electronics store | For LED data line |
| Capacitor (optional) | 100-470µF, 16V+ | 2 | $0.50 | Electronics store | Power filtering |
| Flyback Diode (optional) | SS34, 1N5819, or UF4007 | 1 | $0.20 | Electronics store | If not on MOSFET module |

**Component Kit Option:**
- Generic resistor/capacitor kit: $5-10
- Includes all needed values plus extras

---

### Wiring & Connectors

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| Hookup Wire | 22-24 AWG, stranded | 3m | $3-5 | Electronics store | Multiple colors |
| Power Wire | 18-20 AWG, stranded | 1m | $2-3 | Electronics store | Red and black |
| Dupont Connectors | Male/Female headers | 20 | $2-3 | AliExpress, Amazon | Crimping tool helpful |
| Heat Shrink Tubing | Assorted sizes | 1 pack | $3-5 | Electronics store | For insulation |
| Breadboard (optional) | 400 or 830 tie-points | 1 | $3-6 | Electronics store | For prototyping |

**Wire Color Recommendations:**
- Red: +5V power
- Black: Ground (GND)
- Yellow/White: Signal (GPIO 22, 23)
- Other: Additional signals

---

### Ferrofluid Display

| Item | Specification | Quantity | Est. Price | Source | Notes |
|------|--------------|----------|-----------|---------|-------|
| Ferrofluid | 10-50ml | 1 | $10-30 | Amazon, specialty | Amount depends on display size |
| Display Container | Clear, flat bottom | 1 | $5-15 | Varies | Glass vial, petri dish, custom |
| Mineral Oil (optional) | Clear, cosmetic grade | 50ml | $5-10 | Pharmacy, cosmetic | Dilutes ferrofluid |

**Ferrofluid Options:**

| Type | Amount | Use Case | Price |
|------|--------|----------|-------|
| Educational | 10ml | Small display, testing | $10-15 |
| **Standard** | **30ml** | **Recommended** | **$20-25** |
| Large | 50ml+ | Big displays | $30-40 |

**Container Ideas:**
- Glass vial with flat bottom (chemistry supply)
- Petri dish (biology supply)
- Watch glass dish
- Custom 3D printed holder
- Small aquarium-style container

---

## Optional Components

### Enhancements

| Item | Purpose | Est. Price | Priority |
|------|---------|-----------|----------|
| Heatsink | Cooling for electromagnet | $2-4 | Low |
| Small Fan (5V) | Active cooling | $2-5 | Low |
| 3.5mm Audio Jack | AUX input port | $1-2 | Medium |
| Status LEDs | Power/status indicators | $1-2 | Low |
| Enclosure Material | 3D printed or acrylic | $5-20 | High |

### Development/Testing

| Item | Purpose | Est. Price |
|------|---------|-----------|
| Multimeter | Testing voltage/continuity | $10-30 |
| Bench Power Supply | Stable 5V for testing | $20-50 |
| Soldering Iron | Permanent connections | $15-40 |
| Wire Stripper | Clean wire prep | $5-15 |
| Crimping Tool | Dupont connectors | $10-25 |

---

## Tools Required

### Essential

- [ ] Screwdriver set (Phillips, flat)
- [ ] Wire cutters
- [ ] Wire strippers
- [ ] Needle-nose pliers
- [ ] Multimeter (for testing)

### Helpful

- [ ] Soldering iron & solder
- [ ] Helping hands / PCB holder
- [ ] Heat gun (for heat shrink)
- [ ] Label maker
- [ ] Crimping tool (for Dupont connectors)

---

## Where to Buy

### Online Retailers

**International:**
- **AliExpress** - Cheapest, slow shipping (2-4 weeks)
- **Amazon** - Fast shipping, higher prices
- **eBay** - Varies by seller
- **Banggood** - Good electronics selection
- **Adafruit** - Quality components, US-based
- **SparkFun** - Great for learning, tutorials

**Regional:**
- **North America:** Digi-Key, Mouser, Adafruit, SparkFun
- **Europe:** RS Components, Farnell, Conrad Electronic
- **Asia:** Taobao, local electronics markets

### Local Options

- Electronics hobby stores
- Radio Shack (where available)
- University surplus stores
- Maker spaces (often have parts bins)
- Local electronics markets

---

## Cost Breakdown by Category

### Minimum Build (Basic Functionality)

| Category | Cost |
|----------|------|
| ESP32-A1S | $20 |
| 4x18650 + Holder | $15 |
| MOSFET Module | $3 |
| Electromagnet | $7 |
| LED Ring (24) | $8 |
| Resistors/Wire | $5 |
| Breadboard Power | $3 |
| USB Cable | $2 |
| Ferrofluid + Container | $25 |
| **Total** | **~$88** |

### Recommended Build (Better Components)

| Category | Cost |
|----------|------|
| ESP32-A1S | $25 |
| Quality 18650s + Holder | $25 |
| MOSFET Module | $4 |
| Medium Electromagnet | $10 |
| LED Ring (24) | $10 |
| Components/Wire/Connectors | $10 |
| Breadboard Power | $4 |
| USB Cable | $3 |
| Ferrofluid (30ml) | $25 |
| Container | $10 |
| **Total** | **~$126** |

### Deluxe Build (All Options)

| Category | Cost |
|----------|------|
| ESP32-A1S | $25 |
| High-capacity 18650s + Holder | $30 |
| MOSFET Module | $4 |
| Large Electromagnet | $15 |
| LED Ring (32) | $12 |
| Premium Components | $15 |
| Heatsink + Fan | $7 |
| 3D Printed Enclosure | $15 |
| Quality Ferrofluid (50ml) | $35 |
| Custom Container | $15 |
| **Total** | **~$173** |

---

## Battery Charger

**Not included in main BOM but essential:**

| Item | Type | Price | Notes |
|------|------|-------|-------|
| Smart Charger | 4-bay, Li-ion | $15-30 | Nitecore, XTAR, etc. |
| USB Charger | 18650 compatible | $8-15 | Convenient but slower |

**Note:** Most 4x18650 holders have built-in Micro-USB charging, so external charger is optional but recommended for longevity.

**Recommended Features:**
- Individual cell monitoring
- Multiple charge rates (0.5A, 1A, 2A)
- Status display
- Auto-shutoff when full
- Compatible with protected cells

**Charger Brands:**
- Nitecore (high quality, reliable)
- XTAR (good value)
- Opus (feature-rich)
- LiitoKala (budget option)

---

## Bulk Discounts

If building multiple units or group buy:

| Item | Single | 5-Pack | 10-Pack | Savings |
|------|--------|--------|---------|---------|
| ESP32-A1S | $20 | $90 | $160 | 20% |
| LED Rings | $8 | $35 | $65 | 19% |
| MOSFET Modules | $3 | $12 | $20 | 33% |
| Electromagnets | $8 | $35 | $60 | 25% |

---

## Shopping List Template

Copy this checklist when ordering:

```
[ ] ESP32-A1S Audio Kit v2.2
[ ] 4x 18650 batteries (protected)
[ ] 4x18650 holder with USB-A output & Micro-USB charging
[ ] USB-A cable (for battery to breadboard)
[ ] Breadboard power module (with master switch)
[ ] MOSFET trigger module (with diode)
[ ] Electromagnet (5V, 1-2A)
[ ] WS2812B LED ring (24 LEDs)
[ ] 330Ω resistor (1x)
[ ] Hookup wire (22-24 AWG, 3m)
[ ] Power wire (18-20 AWG, 1m)
[ ] Dupont connectors (20+)
[ ] Heat shrink tubing
[ ] Ferrofluid (30ml)
[ ] Display container
[ ] Micro USB cable (for programming)

Optional:
[ ] Capacitors (100µF, 470µF)
[ ] 3.5mm audio jack
[ ] Heatsink for coil
[ ] 5V fan
[ ] Enclosure material
```

---

## Substitutions & Alternatives

### ESP32 Board
- ❌ ESP32 DevKit (lacks audio codec)
- ✅ ESP32-A1S v2.2 (recommended)
- ⚠️ Other ESP32-Audio-Kit versions (may need code changes)

### Batteries
- ✅ 18650 protected cells (recommended)
- ⚠️ 18650 unprotected (only if you know what you're doing)
- ⚠️ Power bank with 5V output (possible but less portable)
- ❌ AA batteries (insufficient current)

### MOSFET Module
- ✅ AOD4184 module (recommended)
- ✅ IRF520 module (if has flyback diode)
- ⚠️ Standalone MOSFET (need to add diode yourself)
- ❌ Relay module (too slow, will click)

### LEDs
- ✅ WS2812B (recommended, most common)
- ✅ SK6812 (compatible, RGBW option)
- ⚠️ APA102 (faster but needs 2 pins)
- ❌ Standard RGB LEDs (not addressable)

---

## Quality Tips

### Don't Cheap Out On:
1. **Batteries** - Safety critical, affects performance
2. **ESP32-A1S** - Clones may have issues
3. **Battery holder** - Need reliable 5V output

### OK to Economize:
1. Resistors - Generic is fine
2. Wire - Any copper stranded wire works
3. Connectors - Function over brand

### Avoid:
1. Unbranded lithium batteries
2. "Free shipping from unknown country" listings
3. Prices way below market average
4. Listings with stock photos only

---

## Inventory Checklist

Before starting assembly, verify you have:

**Electronics:**
- [ ] ESP32-A1S board with pin headers
- [ ] MOSFET module tested (LED lights up)
- [ ] LED ring responds to power (lights up)
- [ ] Electromagnet tested (gets warm when powered)

**Power:**
- [ ] Batteries charged to 4.0V+ each
- [ ] Battery holder outputs 5V at USB-A port
- [ ] Power module outputs 5V on both rails
- [ ] Breadboard master switch works

**Components:**
- [ ] 330Ω resistor measured (correct value)
- [ ] Wire properly stripped
- [ ] Connectors crimped or soldered
- [ ] Heat shrink available

**Tools:**
- [ ] Multimeter available
- [ ] Computer with Arduino IDE installed
- [ ] USB cable connects to ESP32
- [ ] Work surface cleared

---

## Version History

- **v2.0** - Updated for battery-powered design with 4x18650
- **v1.0** - Original USB-C PD version

---

## Need Help?

**Can't find a part?**
- Ask in GitHub Discussions
- Check MakerWorld project page
- Post in Issues with your location

**Part recommendations needed?**
- Specify your country/region
- Mention budget constraints
- List what you've already found

---

**Project**: [FerroWave](https://github.com/matoslav/FerroWave)  
**Author**: Makarov87  
**Crowdfunded by**: [MakerWorld Community](https://makerworld.com/en/crowdfunding/70-ferrowave)  
**License**: MIT

*Thank you to all MakerWorld backers who made this project possible!* 🙏
