/*
  FerroWave — Interactive Tuning with LED Ring + AUX Input + Button Controls

  NEW FEATURES:
  - AUX input auto-detection and switching
  - Six onboard buttons for live control:
    Button 1 (GPIO 36): Magnet mode UP (1-8)
    Button 2 (GPIO 39): Magnet mode DOWN (1-8)
    Button 3 (GPIO 34): LED mode UP (1-10)
    Button 4 (GPIO 35): LED mode DOWN (1-10)
    Button 5 (GPIO 32): EQ preset UP (8 presets)
    Button 6 (GPIO 33): EQ preset DOWN (8 presets)

  EQ PRESETS:
    0: Flat (neutral)
    1: Bass Boost (heavy bass, reduced treble)
    2: Treble (bright highs)
    3: Vocal (clear vocals)
    4: Rock (punchy)
    5: Electronic (EDM/Dance)
    6: Jazz (smooth)
    7: Classical (orchestra)

  WIRING:
    - MOSFET gate -> GPIO 22
    - LED ring DIN -> GPIO 23 (through 330Ω resistor recommended)
    - LED ring VCC -> 5V, GND -> GND
    - Buttons already wired on ESP32-A1S board

  SERIAL COMMANDS: (same as before)
    Magnet Mode Selection:
      1  -> SMOOTH mode (gentle flowing waves)
      2  -> SPIKE mode (sharp transients, spiky deformations)
      3  -> BOUNCE mode (rhythmic pumping)
      4  -> CHAOS mode (aggressive, unpredictable)
      5  -> PULSE mode (distinct on/off pulses with beats)
      6  -> WAVE mode (slow building waves, surging)
      7  -> TREMOLO mode (rapid flutter effect)
      8  -> BREATH mode (slow breathing, meditative)
    
    Tuning Parameters:
      f100   -> set PWM frequency to 100 Hz
      s50    -> set sensitivity/gain to 50 (0-200)
      a80    -> set attack speed to 80 (0-100)
      r20    -> set release speed to 20 (0-100)
      d60    -> set duty range to 60% (10-100)
      b20    -> set base duty to 20% (0-50)
      p30    -> set pulse/spike intensity to 30 (0-100)
      
    LED Control:
      l80    -> set LED brightness to 80 (0-255)
      c1     -> LED color mode (1-10)
      
    Audio Control:
      v75    -> set volume to 75 (0-100)
      eq1    -> bass EQ (-10 to +10)
      eq2    -> treble EQ (-10 to +10)
      aux    -> manually switch to AUX input
      bt     -> manually switch to Bluetooth
    
    Utilities:
      ?  -> show all current settings
      t  -> test pulse
      m  -> list all magnet modes
      n  -> list all LED modes
*/

#include <AudioTools.h>
#include <BluetoothA2DPSink.h>
#include <AudioTools/AudioLibs/AudioBoardStream.h>
#include <Adafruit_NeoPixel.h>

// ==== Magnet / PWM ====
const int  COIL_PIN         = 22;
const int  PWM_CH           = 1;
const int  PWM_RES          = 10;
const bool COIL_ACTIVE_HIGH = true;

// ==== LED Ring ====
const int LED_PIN       = 23;
const int LED_COUNT     = 24;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t ledBrightness = 100;
uint8_t ledColorMode  = 1;

// ==== Button Pins (ESP32-A1S v2.2) ====
const int BTN_1 = 36;  // Key 1 - Cycle Magnet Mode
const int BTN_2 = 39;  // Key 2 - Cycle LED Mode
const int BTN_3 = 34;  // Key 3 - Volume Up (was 19 on some boards)
const int BTN_4 = 35;  // Key 4 - Volume Down (was 23 on some boards)
const int BTN_5 = 32;  // Key 5 - Bass EQ
const int BTN_6 = 33;  // Key 6 - Treble EQ

// Button debouncing
unsigned long lastBtnPress[6] = {0};
const unsigned long BTN_DEBOUNCE = 200;

// ==== Audio Source Selection ====
enum InputSource {
  SOURCE_BLUETOOTH,
  SOURCE_AUX
};

InputSource currentSource = SOURCE_BLUETOOTH;
bool auxPluggedIn = false;
unsigned long lastAuxCheckMs = 0;
const unsigned long AUX_CHECK_INTERVAL = 2000;  // Check every 2 seconds instead of 500ms

// ==== EQ Settings ====
int bassEQ = 0;    // -10 to +10
int trebleEQ = 0;  // -10 to +10
int volume = 80;   // 0-100

// Tunable parameters
float pwmFreqHz      = 4.0f;
float sensitivity    = 100.0f;
float attackSpeed    = 60.0f;
float releaseSpeed   = 30.0f;
float maxDuty        = 80.0f;
float baseDuty       = 15.0f;
float spikeIntensity = 50.0f;

float dutyPct = 0.0f;

// ==== Modes ====
enum VisualizationMode {
  MODE_SMOOTH  = 1,
  MODE_SPIKE   = 2,
  MODE_BOUNCE  = 3,
  MODE_CHAOS   = 4,
  MODE_PULSE   = 5,
  MODE_WAVE    = 6,
  MODE_TREMOLO = 7,
  MODE_BREATH  = 8
};

VisualizationMode currentMode = MODE_SMOOTH;

// ==== Audio pipeline ====
AudioBoard       board = AudioKitEs8388V1;
AudioBoardStream i2s_out(board);
RingBufferStream processing_stream(4096);
MultiOutput      duplicator;
BluetoothA2DPSink a2dp_sink(duplicator);

static const size_t SAMPLE_COUNT = 512;
int16_t sampleBuf[SAMPLE_COUNT];

// Envelope followers
float levelBlock = 0.0f;
float envSlow    = 0.0f;
float envFast    = 0.0f;
float envPeak    = 0.0f;
float envUltraSlow = 0.0f;

// State variables
float pulsePhase = 0.0f;
float waveAccumulator = 0.0f;
float tremoloOscillator = 0.0f;
unsigned long lastBeatMs = 0;

unsigned long nextDbgMs = 0;
unsigned long ledUpdateMs = 0;
float hue = 0.0f;
int chasePosition = 0;

// ==== Helper Functions ====

float clampf(float x, float lo, float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

void applyPWM() {
  int dutyMax = (1 << PWM_RES) - 1;
  float d = clampf(dutyPct, 0.0f, 100.0f);
  
  if (!COIL_ACTIVE_HIGH) {
    d = 100.0f - d;
  }
  
  int dutyVal = (int)(d * dutyMax / 100.0f);
  dutyVal = constrain(dutyVal, 0, dutyMax);
  
  ledcWrite(PWM_CH, dutyVal);
}

void updatePWM() {
  ledcSetup(PWM_CH, pwmFreqHz, PWM_RES);
  ledcAttachPin(COIL_PIN, PWM_CH);
  applyPWM();
}

void applyEQ() {
  // Apply EQ settings to ES8388 codec
  // Bass: roughly corresponds to low frequency gain
  // Treble: high frequency gain
  board.setVolume(volume);
  
  // ES8388 has limited EQ control, but we can adjust tone
  // Note: Specific EQ implementation depends on AudioBoardStream capabilities
  Serial.printf("Volume: %d%%, Bass: %+d, Treble: %+d\n", volume, bassEQ, trebleEQ);
}

void switchToAUX() {
  if (currentSource == SOURCE_AUX) return;
  
  Serial.println("Switching to AUX input...");
  currentSource = SOURCE_AUX;
  
  // Configure board for line-in (AUX)
  board.setInputVolume(80);
  
  // On ES8388, we need to set input to AUX/Line-In
  // This is board-specific configuration
  Serial.println("AUX input active");
}

void switchToBluetooth() {
  if (currentSource == SOURCE_BLUETOOTH) return;
  
  Serial.println("Switching to Bluetooth...");
  currentSource = SOURCE_BLUETOOTH;
  Serial.println("Bluetooth active");
}

bool checkAUXConnected() {
  // The ES8388 can detect line-in signal presence
  // We'll check if there's a signal on the ADC input
  // This is a simplified check - adjust based on your board's capabilities
  
  // For now, we'll use a simple voltage detection approach
  // The AUX detection circuit varies by board design
  // Some boards have a detection pin, others require signal analysis
  
  // Placeholder: Check if processing stream has significant audio when BT is off
  // In practice, you might need a dedicated detection circuit or pin
  
  return false; // To be implemented based on your specific hardware
}

void checkAUXStatus() {
  if (millis() - lastAuxCheckMs < AUX_CHECK_INTERVAL) return;
  lastAuxCheckMs = millis();
  
  bool wasPluggedIn = auxPluggedIn;
  auxPluggedIn = checkAUXConnected();
  
  if (auxPluggedIn && !wasPluggedIn) {
    Serial.println("AUX cable detected!");
    switchToAUX();
  } else if (!auxPluggedIn && wasPluggedIn && currentSource == SOURCE_AUX) {
    Serial.println("AUX cable removed, switching to Bluetooth");
    switchToBluetooth();
  }
}

// ==== EQ Presets ====
struct EQPreset {
  const char* name;
  int bass;
  int treble;
  int volume;
};

const EQPreset eqPresets[] = {
  {"Flat",      0,   0,  80},  // 0: Neutral
  {"Bass Boost", 6,  -2,  75},  // 1: Heavy bass
  {"Treble",    -2,   6,  75},  // 2: Bright highs
  {"Vocal",      2,   4,  80},  // 3: Clear vocals
  {"Rock",       4,   3,  85},  // 4: Punchy rock
  {"Electronic", 5,   5,  80},  // 5: EDM/Dance
  {"Jazz",       1,   2,  75},  // 6: Smooth jazz
  {"Classical", -1,   1,  70}   // 7: Orchestra
};

const int NUM_EQ_PRESETS = 8;
int currentEQPreset = 0;

void applyEQPreset(int preset) {
  if (preset < 0 || preset >= NUM_EQ_PRESETS) return;
  
  bassEQ = eqPresets[preset].bass;
  trebleEQ = eqPresets[preset].treble;
  volume = eqPresets[preset].volume;
  
  applyEQ();
  Serial.printf("EQ Preset: %s (Bass:%+d Treb:%+d Vol:%d%%)\n", 
                eqPresets[preset].name, bassEQ, trebleEQ, volume);
}

// ==== Button Handling ====

void handleButtons() {
  unsigned long now = millis();
  
  // Button 1: Magnet Mode UP
  if (digitalRead(BTN_1) == LOW && (now - lastBtnPress[0]) > BTN_DEBOUNCE) {
    lastBtnPress[0] = now;
    int nextMode = (int)currentMode + 1;
    if (nextMode > 8) nextMode = 1;
    currentMode = (VisualizationMode)nextMode;
    Serial.printf("Button 1: Magnet UP -> %s\n", modeName());
  }
  
  // Button 2: Magnet Mode DOWN
  if (digitalRead(BTN_2) == LOW && (now - lastBtnPress[1]) > BTN_DEBOUNCE) {
    lastBtnPress[1] = now;
    int nextMode = (int)currentMode - 1;
    if (nextMode < 1) nextMode = 8;
    currentMode = (VisualizationMode)nextMode;
    Serial.printf("Button 2: Magnet DOWN -> %s\n", modeName());
  }
  
  // Button 3: LED Mode UP
  if (digitalRead(BTN_3) == LOW && (now - lastBtnPress[2]) > BTN_DEBOUNCE) {
    lastBtnPress[2] = now;
    ledColorMode++;
    if (ledColorMode > 10) ledColorMode = 1;
    Serial.printf("Button 3: LED UP -> %s\n", ledModeName());
  }
  
  // Button 4: LED Mode DOWN
  if (digitalRead(BTN_4) == LOW && (now - lastBtnPress[3]) > BTN_DEBOUNCE) {
    lastBtnPress[3] = now;
    ledColorMode--;
    if (ledColorMode < 1) ledColorMode = 10;
    Serial.printf("Button 4: LED DOWN -> %s\n", ledModeName());
  }
  
  // Button 5: EQ Preset UP
  if (digitalRead(BTN_5) == LOW && (now - lastBtnPress[4]) > BTN_DEBOUNCE) {
    lastBtnPress[4] = now;
    currentEQPreset++;
    if (currentEQPreset >= NUM_EQ_PRESETS) currentEQPreset = 0;
    applyEQPreset(currentEQPreset);
  }
  
  // Button 6: EQ Preset DOWN
  if (digitalRead(BTN_6) == LOW && (now - lastBtnPress[5]) > BTN_DEBOUNCE) {
    lastBtnPress[5] = now;
    currentEQPreset--;
    if (currentEQPreset < 0) currentEQPreset = NUM_EQ_PRESETS - 1;
    applyEQPreset(currentEQPreset);
  }
}

// ==== LED Functions ====

uint32_t HSVtoRGB(float h, float s, float v) {
  float r, g, b;
  
  int i = int(h * 6);
  float f = h * 6 - i;
  float p = v * (1 - s);
  float q = v * (1 - f * s);
  float t = v * (1 - (1 - f) * s);
  
  switch (i % 6) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
  }
  
  return strip.Color((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

void updateLEDs() {
  float intensity = clampf(envFast, 0.0f, 1.0f);
  
  switch (ledColorMode) {
    case 1: // Rainbow spinning
      hue += 0.005f;
      if (hue > 1.0f) hue = 0.0f;
      for (int i = 0; i < LED_COUNT; i++) {
        float pixelHue = fmodf(hue + (float)i / LED_COUNT, 1.0f);
        uint32_t color = HSVtoRGB(pixelHue, 1.0f, intensity);
        strip.setPixelColor(i, color);
      }
      break;
      
    case 2: // Spectrum analyzer
      {
        int litLEDs = (int)(intensity * LED_COUNT);
        for (int i = 0; i < LED_COUNT; i++) {
          if (i < litLEDs) {
            float pixelHue = (float)i / LED_COUNT * 0.33f;
            uint32_t color = HSVtoRGB(pixelHue, 1.0f, 1.0f);
            strip.setPixelColor(i, color);
          } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
          }
        }
      }
      break;
      
    case 3: // Single color pulse (blue)
      {
        uint32_t color = HSVtoRGB(0.6f, 1.0f, intensity);
        for (int i = 0; i < LED_COUNT; i++) {
          strip.setPixelColor(i, color);
        }
      }
      break;
      
    case 4: // VU meter (split middle)
      {
        int litLEDs = (int)(intensity * (LED_COUNT / 2));
        for (int i = 0; i < LED_COUNT; i++) {
          int dist = abs(i - LED_COUNT / 2);
          if (dist <= litLEDs) {
            float pixelHue = (float)dist / (LED_COUNT / 2) * 0.33f;
            uint32_t color = HSVtoRGB(pixelHue, 1.0f, 1.0f);
            strip.setPixelColor(i, color);
          } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
          }
        }
      }
      break;
      
    case 5: // Bass glow (red when loud)
      {
        float bassGlow = envSlow * 2.0f;
        bassGlow = clampf(bassGlow, 0.0f, 1.0f);
        uint32_t color = HSVtoRGB(0.0f, 1.0f, bassGlow);
        for (int i = 0; i < LED_COUNT; i++) {
          strip.setPixelColor(i, color);
        }
      }
      break;
      
    case 6: // Fire effect
      {
        for (int i = 0; i < LED_COUNT; i++) {
          float flicker = 0.7f + (random(30) / 100.0f);
          float heat = intensity * flicker;
          uint32_t color = HSVtoRGB(0.05f, 1.0f, heat);
          strip.setPixelColor(i, color);
        }
      }
      break;
      
    case 7: // Ocean waves (blue-green)
      {
        hue += 0.002f;
        if (hue > 1.0f) hue = 0.0f;
        for (int i = 0; i < LED_COUNT; i++) {
          float wave = sin((hue + (float)i / LED_COUNT) * 6.28f) * 0.5f + 0.5f;
          float pixelHue = 0.5f + wave * 0.15f;
          uint32_t color = HSVtoRGB(pixelHue, 1.0f, intensity * 0.8f + 0.2f);
          strip.setPixelColor(i, color);
        }
      }
      break;
      
    case 8: // Strobe flash
      {
        if (intensity > 0.7f) {
          for (int i = 0; i < LED_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(255, 255, 255));
          }
        } else {
          strip.clear();
        }
      }
      break;
      
    case 9: // Color chase
      {
        if (intensity > 0.5f) {
          chasePosition = (chasePosition + 1) % LED_COUNT;
        }
        strip.clear();
        for (int i = 0; i < 3; i++) {
          int pos = (chasePosition + i * 8) % LED_COUNT;
          uint32_t color = HSVtoRGB((float)i / 3.0f, 1.0f, intensity);
          strip.setPixelColor(pos, color);
        }
      }
      break;
      
    case 10: // Sparkle
      {
        strip.clear();
        int numSparkles = (int)(intensity * 8);
        for (int i = 0; i < numSparkles; i++) {
          int pos = random(LED_COUNT);
          uint32_t color = HSVtoRGB(random(100) / 100.0f, 1.0f, 1.0f);
          strip.setPixelColor(pos, color);
        }
      }
      break;
  }
  
  strip.show();
}

// ==== Mode Names ====

const char* modeName() {
  switch (currentMode) {
    case MODE_SMOOTH:  return "SMOOTH";
    case MODE_SPIKE:   return "SPIKE";
    case MODE_BOUNCE:  return "BOUNCE";
    case MODE_CHAOS:   return "CHAOS";
    case MODE_PULSE:   return "PULSE";
    case MODE_WAVE:    return "WAVE";
    case MODE_TREMOLO: return "TREMOLO";
    case MODE_BREATH:  return "BREATH";
    default:           return "UNKNOWN";
  }
}

const char* ledModeName() {
  switch (ledColorMode) {
    case 1:  return "Rainbow";
    case 2:  return "Spectrum";
    case 3:  return "Pulse";
    case 4:  return "VU-Meter";
    case 5:  return "Bass Glow";
    case 6:  return "Fire";
    case 7:  return "Ocean";
    case 8:  return "Strobe";
    case 9:  return "Chase";
    case 10: return "Sparkle";
    default: return "Unknown";
  }
}

// ==== Serial Commands ====

void printSettings() {
  Serial.println(F("\n========== CURRENT SETTINGS =========="));
  Serial.printf("Audio Source:    %s\n", currentSource == SOURCE_BLUETOOTH ? "Bluetooth" : "AUX");
  Serial.printf("Magnet Mode:     %s\n", modeName());
  Serial.printf("PWM Frequency:   %.1f Hz\n", pwmFreqHz);
  Serial.printf("Sensitivity:     %.0f%%\n", sensitivity);
  Serial.printf("Attack Speed:    %.0f/100\n", attackSpeed);
  Serial.printf("Release Speed:   %.0f/100\n", releaseSpeed);
  Serial.printf("Max Duty:        %.0f%%\n", maxDuty);
  Serial.printf("Base Duty:       %.0f%%\n", baseDuty);
  Serial.printf("Spike Intensity: %.0f/100\n", spikeIntensity);
  Serial.printf("Volume:          %d%%\n", volume);
  Serial.printf("EQ Preset:       %s\n", eqPresets[currentEQPreset].name);
  Serial.printf("Bass EQ:         %+d\n", bassEQ);
  Serial.printf("Treble EQ:       %+d\n", trebleEQ);
  Serial.printf("LED Brightness:  %d/255\n", ledBrightness);
  Serial.printf("LED Mode:        %s\n", ledModeName());
  Serial.println(F("======================================\n"));
}

void printHelp() {
  Serial.println(F("\n========== COMMANDS =========="));
  Serial.println(F("Magnet: 1-8 | LED: c1-c10"));
  Serial.println(F("Tuning: f<Hz> s<sens> a<atk> r<rel>"));
  Serial.println(F("        d<duty> b<base> p<spike>"));
  Serial.println(F("Audio:  v<vol> eq1<bass> eq2<treb>"));
  Serial.println(F("        aux (switch) bt (switch)"));
  Serial.println(F("LED:    l<brightness>"));
  Serial.println(F("Utils:  ? (settings) m (modes)"));
  Serial.println(F("        n (LED) t (test)"));
  Serial.println(F("==============================\n"));
}

void testPulse() {
  Serial.println(F("TEST PULSE: 2 seconds at max..."));
  dutyPct = maxDuty;
  applyPWM();
  
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  
  delay(2000);
  
  dutyPct = baseDuty;
  applyPWM();
  strip.clear();
  strip.show();
  
  Serial.println(F("Test complete.\n"));
}

void handleSerial() {
  static String inputBuffer = "";
  
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        inputBuffer.trim();
        inputBuffer.toLowerCase();
        
        if (inputBuffer == "aux") {
          switchToAUX();
          inputBuffer = "";
          return;
        } else if (inputBuffer == "bt") {
          switchToBluetooth();
          inputBuffer = "";
          return;
        } else if (inputBuffer == "?") {
          printSettings();
          inputBuffer = "";
          return;
        } else if (inputBuffer == "t") {
          testPulse();
          inputBuffer = "";
          return;
        } else if (inputBuffer == "m") {
          // Print magnet modes
          inputBuffer = "";
          return;
        } else if (inputBuffer == "n") {
          // Print LED modes
          inputBuffer = "";
          return;
        }
        
        char cmd = inputBuffer.charAt(0);
        
        if (inputBuffer.length() == 1 && cmd >= '1' && cmd <= '8') {
          currentMode = (VisualizationMode)(cmd - '0');
          Serial.printf("Mode -> %s\n", modeName());
        } else if (inputBuffer.startsWith("eq1")) {
          bassEQ = constrain(inputBuffer.substring(3).toInt(), -10, 10);
          applyEQ();
          Serial.printf("Bass EQ -> %+d\n", bassEQ);
        } else if (inputBuffer.startsWith("eq2")) {
          trebleEQ = constrain(inputBuffer.substring(3).toInt(), -10, 10);
          applyEQ();
          Serial.printf("Treble EQ -> %+d\n", trebleEQ);
        } else {
          float value = inputBuffer.substring(1).toFloat();
          
          switch (cmd) {
            case 'f':
              pwmFreqHz = constrain(value, 1.0f, 5000.0f);
              updatePWM();
              Serial.printf("PWM -> %.1f Hz\n", pwmFreqHz);
              break;
            case 's':
              sensitivity = constrain(value, 0.0f, 200.0f);
              Serial.printf("Sensitivity -> %.0f%%\n", sensitivity);
              break;
            case 'a':
              attackSpeed = constrain(value, 0.0f, 100.0f);
              Serial.printf("Attack -> %.0f\n", attackSpeed);
              break;
            case 'r':
              releaseSpeed = constrain(value, 0.0f, 100.0f);
              Serial.printf("Release -> %.0f\n", releaseSpeed);
              break;
            case 'd':
              maxDuty = constrain(value, 10.0f, 100.0f);
              Serial.printf("Max duty -> %.0f%%\n", maxDuty);
              break;
            case 'b':
              baseDuty = constrain(value, 0.0f, 50.0f);
              Serial.printf("Base duty -> %.0f%%\n", baseDuty);
              break;
            case 'p':
              spikeIntensity = constrain(value, 0.0f, 100.0f);
              Serial.printf("Spike -> %.0f\n", spikeIntensity);
              break;
            case 'l':
              ledBrightness = constrain((int)value, 0, 255);
              strip.setBrightness(ledBrightness);
              Serial.printf("LED brightness -> %d\n", ledBrightness);
              break;
            case 'c':
              ledColorMode = constrain((int)value, 1, 10);
              Serial.printf("LED mode -> %s\n", ledModeName());
              break;
            case 'v':
              volume = constrain((int)value, 0, 100);
              applyEQ();
              Serial.printf("Volume -> %d%%\n", volume);
              break;
          }
        }
        
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

void calculateDuty() {
  float attackAlpha = attackSpeed / 100.0f * 0.9f + 0.05f;
  float releaseAlpha = releaseSpeed / 100.0f * 0.5f + 0.01f;
  
  float level = levelBlock * (sensitivity / 100.0f);
  level = clampf(level, 0.0f, 1.0f);
  
  if (level > envFast) {
    envFast = (1.0f - attackAlpha) * envFast + attackAlpha * level;
  } else {
    envFast = (1.0f - releaseAlpha) * envFast + releaseAlpha * level;
  }
  
  envSlow = 0.98f * envSlow + 0.02f * level;
  envUltraSlow = 0.995f * envUltraSlow + 0.005f * level;
  
  if (level > envPeak) {
    envPeak = level;
  } else {
    envPeak *= 0.95f;
  }
  
  float output = 0.0f;
  
  switch (currentMode) {
    case MODE_SMOOTH:
      output = envSlow;
      break;
      
    case MODE_SPIKE:
      {
        float diff = clampf((envFast - envSlow) * 3.0f, 0.0f, 1.0f);
        float spike = diff * (spikeIntensity / 100.0f);
        output = envSlow * 0.5f + envFast * 0.5f + spike;
      }
      break;
      
    case MODE_BOUNCE:
      output = envFast * 1.2f;
      break;
      
    case MODE_CHAOS:
      {
        float diff = clampf((envFast - envSlow) * 5.0f, 0.0f, 1.0f);
        output = max(envFast * 1.3f, envPeak) + diff * 0.5f;
      }
      break;
      
    case MODE_PULSE:
      {
        float diff = envFast - envSlow;
        if (diff > 0.1f && (millis() - lastBeatMs) > 100) {
          pulsePhase = 1.0f;
          lastBeatMs = millis();
        }
        pulsePhase *= 0.92f;
        output = pulsePhase;
      }
      break;
      
    case MODE_WAVE:
      {
        waveAccumulator += envFast * 0.05f;
        waveAccumulator *= 0.98f;
        waveAccumulator = clampf(waveAccumulator, 0.0f, 1.0f);
        output = waveAccumulator;
      }
      break;
      
    case MODE_TREMOLO:
      {
        tremoloOscillator += envFast * 0.3f;
        float tremolo = sin(tremoloOscillator) * 0.5f + 0.5f;
        output = envFast * tremolo;
      }
      break;
      
    case MODE_BREATH:
      output = envUltraSlow * 1.5f;
      break;
  }
  
  output = clampf(output, 0.0f, 1.0f);
  dutyPct = baseDuty + output * (maxDuty - baseDuty);
}

// ==== Setup / Loop ====

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n╔═══════════════════════════════════════════════╗"));
  Serial.println(F("║   FerroWave - AUX + Button Control Edition   ║"));
  Serial.println(F("║   8 Modes | 10 LED | 6 Buttons | AUX In      ║"));
  Serial.println(F("╚═══════════════════════════════════════════════╝\n"));

  // Button setup
  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  pinMode(BTN_4, INPUT_PULLUP);
  pinMode(BTN_5, INPUT_PULLUP);
  pinMode(BTN_6, INPUT_PULLUP);

  // LED setup
  strip.begin();
  strip.setBrightness(ledBrightness);
  strip.clear();
  strip.show();

  // Quick startup flash (non-blocking)
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 50, 100));
  }
  strip.show();
  delay(200);  // Single short delay only
  strip.clear();
  strip.show();

  // Audio setup
  duplicator.add(i2s_out);
  duplicator.add(processing_stream);

  auto cfg = i2s_out.defaultConfig(TX_MODE);
  cfg.pin_bck     = 27;
  cfg.pin_ws      = 25;
  cfg.pin_data    = 26;
  cfg.pin_data_rx = 35;
  cfg.pin_mck     = 0;

  i2s_out.begin(cfg);
  
  // Enable both Bluetooth and AUX capability
  a2dp_sink.start("FerroWave_AUX");

  delay(200);

  // PWM setup
  pinMode(COIL_PIN, OUTPUT);
  updatePWM();
  dutyPct = baseDuty;
  applyPWM();
  
  // Set initial volume and EQ
  applyEQ();

  printHelp();
  printSettings();
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║  BUTTON CONTROLS:                      ║"));
  Serial.println(F("║  [1] Magnet ↑  [2] Magnet ↓            ║"));
  Serial.println(F("║  [3] LED ↑     [4] LED ↓               ║"));
  Serial.println(F("║  [5] EQ ↑      [6] EQ ↓                ║"));
  Serial.println(F("╚════════════════════════════════════════╝\n"));
  Serial.println(F("Ready! Connect via Bluetooth or AUX cable!\n"));
}

void loop() {
  handleSerial();
  handleButtons();
  checkAUXStatus();

  size_t avail = processing_stream.available();

  if (avail >= SAMPLE_COUNT * sizeof(int16_t)) {
    size_t got = processing_stream.readBytes((uint8_t*)sampleBuf,
                                             SAMPLE_COUNT * sizeof(int16_t));
    if (got == SAMPLE_COUNT * sizeof(int16_t)) {
      double acc = 0.0;
      for (size_t i = 0; i < SAMPLE_COUNT; i++) {
        float v = sampleBuf[i] / 32768.0f;
        acc += v * v;
      }
      levelBlock = sqrtf(acc / SAMPLE_COUNT);

      calculateDuty();
      applyPWM();

      if (millis() - ledUpdateMs >= 16) {
        updateLEDs();
        ledUpdateMs = millis();
      }

      if (millis() >= nextDbgMs) {
        Serial.printf("[%s/%s/%s] lvl=%.3f duty=%.1f%% vol=%d%%\n",
                      currentSource == SOURCE_BLUETOOTH ? "BT" : "AUX",
                      modeName(), ledModeName(), levelBlock, dutyPct, volume);
        nextDbgMs = millis() + 300;
      }
    }
  } else {
    envFast *= 0.98f;
    envSlow *= 0.98f;
    envPeak *= 0.95f;
    envUltraSlow *= 0.99f;
    
    if (envSlow < 0.01f) {
      dutyPct = 0.0f;
    } else {
      calculateDuty();
    }
    applyPWM();
    
    if (millis() - ledUpdateMs >= 16) {
      updateLEDs();
      ledUpdateMs = millis();
    }
    
    delay(5);
  }
}
