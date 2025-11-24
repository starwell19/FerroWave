/*
  FerroWave — Interactive Tuning with LED Ring (Extended Presets)

  - ESP32-A1S v2.2 / ES8388 using AudioBoardStream + MultiOutput
  - Bluetooth A2DP input
  - 5V electromagnet on GPIO 22 via MOSFET
  - WS2812B LED ring (24 LEDs) on GPIO 23
  - SERIAL COMMANDS for live tuning!

  WIRING:
    - MOSFET gate -> GPIO 22
    - LED ring DIN -> GPIO 23 (through 330Ω resistor recommended)
    - LED ring VCC -> 5V, GND -> GND

  SERIAL COMMANDS:
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
      f100   -> set PWM frequency to 100 Hz (try: f4, f50, f500, f1000, f2000)
      s50    -> set sensitivity/gain to 50 (0-200, default 100)
      a80    -> set attack speed to 80 (0-100, higher = faster response)
      r20    -> set release speed to 20 (0-100, higher = faster decay)
      d60    -> set duty range to 60% (10-100, max duty ceiling)
      b20    -> set base duty to 20% (0-50, minimum always-on duty)
      p30    -> set pulse/spike intensity to 30 (0-100)
      
    LED Control:
      l80    -> set LED brightness to 80 (0-255)
      c1     -> LED color mode (1-10, see list below)
      
    LED Modes:
      c1  = Rainbow spin        c6  = Fire effect
      c2  = Spectrum analyzer   c7  = Ocean waves
      c3  = Pulse (blue)        c8  = Strobe flash
      c4  = VU-Meter            c9  = Color chase
      c5  = Bass glow           c10 = Sparkle
    
    Utilities:
      ?  -> show all current settings
      i  -> show parameter info
      t  -> test pulse (2 second full-power pulse + LED flash)
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
const int LED_COUNT     = 24;      // 24 LEDs

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t ledBrightness = 100;       // LED brightness (0-255)
uint8_t ledColorMode  = 1;         // 1-10 different LED effects

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
  MODE_SMOOTH  = 1,  // Gentle, flowing waves
  MODE_SPIKE   = 2,  // Sharp transients, spiky deformations
  MODE_BOUNCE  = 3,  // Rhythmic pumping/bouncing
  MODE_CHAOS   = 4,  // Aggressive, unpredictable
  MODE_PULSE   = 5,  // Distinct on/off pulses with beats
  MODE_WAVE    = 6,  // Slow building waves, surging
  MODE_TREMOLO = 7,  // Rapid flutter/tremolo effect
  MODE_BREATH  = 8   // Slow breathing, meditative
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
float envSlow    = 0.0f;  // Slow average
float envFast    = 0.0f;  // Fast transients
float envPeak    = 0.0f;  // Peak detector
float envUltraSlow = 0.0f; // Very slow for wave building

// State variables for different modes
float pulsePhase = 0.0f;
float waveAccumulator = 0.0f;
float tremoloOscillator = 0.0f;
unsigned long lastBeatMs = 0;

unsigned long nextDbgMs = 0;
unsigned long ledUpdateMs = 0;
float hue = 0.0f;
int chasePosition = 0;

// ==== Helpers ====

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

// HSV to RGB conversion
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
          float pixelHue = 0.5f + wave * 0.15f; // Blue-green range
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

void printMagnetModes() {
  Serial.println(F("\n========== MAGNET MODES =========="));
  Serial.println(F("1 = SMOOTH  - Gentle flowing waves"));
  Serial.println(F("2 = SPIKE   - Sharp transients, spiky"));
  Serial.println(F("3 = BOUNCE  - Rhythmic pumping"));
  Serial.println(F("4 = CHAOS   - Aggressive, unpredictable"));
  Serial.println(F("5 = PULSE   - Distinct on/off beats"));
  Serial.println(F("6 = WAVE    - Slow building surges"));
  Serial.println(F("7 = TREMOLO - Rapid flutter effect"));
  Serial.println(F("8 = BREATH  - Slow meditative breathing"));
  Serial.println(F("==================================\n"));
}

void printLEDModes() {
  Serial.println(F("\n========== LED MODES =========="));
  Serial.println(F("c1  = Rainbow spin"));
  Serial.println(F("c2  = Spectrum analyzer"));
  Serial.println(F("c3  = Pulse (blue)"));
  Serial.println(F("c4  = VU-Meter"));
  Serial.println(F("c5  = Bass glow (red)"));
  Serial.println(F("c6  = Fire effect"));
  Serial.println(F("c7  = Ocean waves"));
  Serial.println(F("c8  = Strobe flash"));
  Serial.println(F("c9  = Color chase"));
  Serial.println(F("c10 = Sparkle"));
  Serial.println(F("===============================\n"));
}

void printSettings() {
  Serial.println(F("\n========== CURRENT SETTINGS =========="));
  Serial.printf("Magnet Mode:     %s\n", modeName());
  Serial.printf("PWM Frequency:   %.1f Hz\n", pwmFreqHz);
  Serial.printf("Sensitivity:     %.0f%%\n", sensitivity);
  Serial.printf("Attack Speed:    %.0f/100\n", attackSpeed);
  Serial.printf("Release Speed:   %.0f/100\n", releaseSpeed);
  Serial.printf("Max Duty:        %.0f%%\n", maxDuty);
  Serial.printf("Base Duty:       %.0f%%\n", baseDuty);
  Serial.printf("Spike Intensity: %.0f/100\n", spikeIntensity);
  Serial.printf("LED Brightness:  %d/255\n", ledBrightness);
  Serial.printf("LED Mode:        %s\n", ledModeName());
  Serial.println(F("======================================\n"));
}

void printHelp() {
  Serial.println(F("\n========== COMMANDS =========="));
  Serial.println(F("Magnet Modes: 1-8 (type 'm' for list)"));
  Serial.println(F("Tuning:"));
  Serial.println(F("  f<num>  PWM frequency (f4, f500, f1000)"));
  Serial.println(F("  s<num>  Sensitivity 0-200"));
  Serial.println(F("  a<num>  Attack speed 0-100"));
  Serial.println(F("  r<num>  Release speed 0-100"));
  Serial.println(F("  d<num>  Max duty 10-100"));
  Serial.println(F("  b<num>  Base duty 0-50"));
  Serial.println(F("  p<num>  Spike intensity 0-100"));
  Serial.println(F("LED:"));
  Serial.println(F("  l<num>  Brightness 0-255"));
  Serial.println(F("  c<num>  Mode 1-10 (type 'n' for list)"));
  Serial.println(F("Utils:"));
  Serial.println(F("  ?  Show settings"));
  Serial.println(F("  m  List magnet modes"));
  Serial.println(F("  n  List LED modes"));
  Serial.println(F("  t  Test pulse"));
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
        char cmd = inputBuffer.charAt(0);
        
        if (inputBuffer.length() == 1) {
          switch (cmd) {
            case '1': currentMode = MODE_SMOOTH;  Serial.println(F("Mode -> SMOOTH")); break;
            case '2': currentMode = MODE_SPIKE;   Serial.println(F("Mode -> SPIKE")); break;
            case '3': currentMode = MODE_BOUNCE;  Serial.println(F("Mode -> BOUNCE")); break;
            case '4': currentMode = MODE_CHAOS;   Serial.println(F("Mode -> CHAOS")); break;
            case '5': currentMode = MODE_PULSE;   Serial.println(F("Mode -> PULSE")); break;
            case '6': currentMode = MODE_WAVE;    Serial.println(F("Mode -> WAVE")); break;
            case '7': currentMode = MODE_TREMOLO; Serial.println(F("Mode -> TREMOLO")); break;
            case '8': currentMode = MODE_BREATH;  Serial.println(F("Mode -> BREATH")); break;
            case '?': printSettings(); break;
            case 'm': printMagnetModes(); break;
            case 'n': printLEDModes(); break;
            case 't': testPulse(); break;
            default:
              Serial.printf("Unknown: %s (type '?' for help)\n", inputBuffer.c_str());
          }
        } else {
          float value = inputBuffer.substring(1).toFloat();
          
          switch (cmd) {
            case 'f':
              pwmFreqHz = constrain(value, 1.0f, 5000.0f);
              updatePWM();
              Serial.printf("PWM frequency -> %.1f Hz\n", pwmFreqHz);
              break;
            case 's':
              sensitivity = constrain(value, 0.0f, 200.0f);
              Serial.printf("Sensitivity -> %.0f%%\n", sensitivity);
              break;
            case 'a':
              attackSpeed = constrain(value, 0.0f, 100.0f);
              Serial.printf("Attack speed -> %.0f/100\n", attackSpeed);
              break;
            case 'r':
              releaseSpeed = constrain(value, 0.0f, 100.0f);
              Serial.printf("Release speed -> %.0f/100\n", releaseSpeed);
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
              Serial.printf("Spike intensity -> %.0f/100\n", spikeIntensity);
              break;
            case 'l':
              ledBrightness = constrain((int)value, 0, 255);
              strip.setBrightness(ledBrightness);
              Serial.printf("LED brightness -> %d/255\n", ledBrightness);
              break;
            case 'c':
              ledColorMode = constrain((int)value, 1, 10);
              Serial.printf("LED mode -> %s\n", ledModeName());
              break;
            default:
              Serial.printf("Unknown: %s\n", inputBuffer.c_str());
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
  
  // Update envelopes
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
  
  // Mode-specific calculations
  switch (currentMode) {
    case MODE_SMOOTH: // Gentle flowing
      output = envSlow;
      break;
      
    case MODE_SPIKE: // Sharp transients
      {
        float diff = clampf((envFast - envSlow) * 3.0f, 0.0f, 1.0f);
        float spike = diff * (spikeIntensity / 100.0f);
        output = envSlow * 0.5f + envFast * 0.5f + spike;
      }
      break;
      
    case MODE_BOUNCE: // Rhythmic pumping
      output = envFast * 1.2f;
      break;
      
    case MODE_CHAOS: // Aggressive max
      {
        float diff = clampf((envFast - envSlow) * 5.0f, 0.0f, 1.0f);
        output = max(envFast * 1.3f, envPeak) + diff * 0.5f;
      }
      break;
      
    case MODE_PULSE: // Distinct on/off pulses
      {
        float diff = envFast - envSlow;
        if (diff > 0.1f && (millis() - lastBeatMs) > 100) {
          pulsePhase = 1.0f;
          lastBeatMs = millis();
        }
        pulsePhase *= 0.92f; // Fast decay
        output = pulsePhase;
      }
      break;
      
    case MODE_WAVE: // Slow building waves
      {
        waveAccumulator += envFast * 0.05f;
        waveAccumulator *= 0.98f;
        waveAccumulator = clampf(waveAccumulator, 0.0f, 1.0f);
        output = waveAccumulator;
      }
      break;
      
    case MODE_TREMOLO: // Rapid flutter
      {
        tremoloOscillator += envFast * 0.3f;
        float tremolo = sin(tremoloOscillator) * 0.5f + 0.5f;
        output = envFast * tremolo;
      }
      break;
      
    case MODE_BREATH: // Slow breathing
      {
        output = envUltraSlow * 1.5f;
      }
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
  Serial.println(F("║   FerroWave - Extended Presets Edition       ║"));
  Serial.println(F("║   8 Magnet Modes | 10 LED Effects            ║"));
  Serial.println(F("╚═══════════════════════════════════════════════╝\n"));

  // LED setup
  strip.begin();
  strip.setBrightness(ledBrightness);
  strip.clear();
  strip.show();

  // Startup animation
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 50, 100));
    strip.show();
    delay(30);
  }
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
  a2dp_sink.start("FerroWave_BackCoil");

  delay(200);

  // PWM setup
  pinMode(COIL_PIN, OUTPUT);
  updatePWM();
  dutyPct = baseDuty;
  applyPWM();

  printHelp();
  printSettings();
  Serial.println(F("Ready! Connect Bluetooth and explore the presets!\n"));
}

void loop() {
  handleSerial();

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
        Serial.printf("[%s/%s] lvl=%.3f duty=%.1f%%\n",
                      modeName(), ledModeName(), levelBlock, dutyPct);
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
