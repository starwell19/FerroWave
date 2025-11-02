/*
  FerroWave — Prototype + Serial Tuning Panel (ESP32-A1S / ES8388)
*/

#include <AudioTools.h>
#include <BluetoothA2DPSink.h>
#include <AudioTools/AudioLibs/AudioBoardStream.h>
#include <NeoPixelBus.h>

// ==== LED ring ====
#define LED_PIN         21
#define LED_COUNT       24
NeoPixelBus<NeoGrbFeature, NeoEsp32BitBang800KbpsMethod> pixels(LED_COUNT, LED_PIN);

// ==== Magnet (PWM) ====
#define MAGNET_PIN      5
#define MAGNET_LEDC_CH  0
float magnetFreqHz = 5.0f;
const float MAGNET_FREQ_MIN = 1.0f;
const float MAGNET_FREQ_MAX = 50.0f;
const float MAGNET_FREQ_FINE_ZONE = 8.0f;
const float MAGNET_STEP_FINE = 0.25f;
const float MAGNET_STEP_COARSE = 1.0f;

// Duty window + base offset (tunable)
float dutyMin = 0.05f;     // 5%
float dutyMax = 0.45f;     // 45%
float baseDutyOffset = 0.00f; // -0.20..+0.20 adds/subtracts from computed duty (then clamped)
bool  magnetEnabled = true;

const int PIN_KEY_UP     = -1;
const int PIN_KEY_DOWN   = -1;
const int PIN_KEY_TOGGLE = -1;
const int PIN_KEY_LED    = -1;
const int PIN_KEY_MODE   = -1;
const int PIN_KEY_SENS   = -1;

struct Btn { int pin; bool last; uint32_t lastMs; };
Btn btns[6];

// ==== Audio Pipeline ====
AudioBoard board = AudioKitEs8388V1;
AudioBoardStream i2s_out(board);
RingBufferStream processing_stream(4096);
MultiOutput duplicator;
BluetoothA2DPSink a2dp_sink(duplicator);
static const size_t SAMPLE_COUNT = 512;
int16_t sampleBuf[SAMPLE_COUNT];

// ==== LEDs / Visuals ====
enum LedMode { LED_OFF=0, LED_RAINBOW=1, LED_VU_METER=2 };
LedMode ledMode = LED_RAINBOW;
bool ledsEnabled = true;
float rainbowOffset = 0.0f;

// Visual tuning
float ledBrightness = 0.60f;      // 0.05..1.00 master brightness
float rainbowBaseSpeed = 0.005f;  // base scroll speed
float rainbowGainSpeed = 0.020f;  // added speed vs level
float vuDecay = 0.06f;            // 0.01..0.20 (larger = faster decay)

// ==== Detection / Sensitivity ====
float levelLPF = 0.0f;
float levelFast = 0.0f;
float sensitivity = 1.0f;    // 0.2..3.0

// Make LPF tunable (was const)
float lpfSlow = 0.02f;   // 0.005..0.200 (magnet)
float lpfFast = 0.20f;   // 0.05..0.80  (LEDs)

// ==== Telemetry ====
bool telemetry = true;           // periodic status every few seconds
uint32_t nextStatMs = 0;
uint32_t statPeriodMs = 3000;

// ==== Utils ====
float clampf(float x, float lo, float hi){ return x < lo ? lo : (x > hi ? hi : x); }

RgbColor colorFromScalar(float t){
  HsbColor c(clampf(t,0.f,1.f), 1.0f, 1.0f);
  return RgbColor(c);
}

void setupMagnetPWM(){
  ledcSetup(MAGNET_LEDC_CH, magnetFreqHz, 10);
  ledcAttachPin(MAGNET_PIN, MAGNET_LEDC_CH);
  uint32_t duty = (uint32_t)(dutyMin * ((1<<10)-1));
  ledcWrite(MAGNET_LEDC_CH, duty);
}

void setMagnetFreq(float hz){
  magnetFreqHz = clampf(hz, MAGNET_FREQ_MIN, MAGNET_FREQ_MAX);
  ledcSetup(MAGNET_LEDC_CH, magnetFreqHz, 10);
}

void updateMagnetDuty(float audioLevel01){
  if(!magnetEnabled){
    ledcWrite(MAGNET_LEDC_CH, 0);
    return;
  }
  // Smooth envelope for magnet, apply sensitivity and base offset, clamp to window
  levelLPF = (1.0f - lpfSlow)*levelLPF + lpfSlow*(audioLevel01 * sensitivity);
  float duty = clampf(levelLPF + baseDutyOffset, dutyMin, dutyMax);
  uint32_t raw = (uint32_t)(duty * ((1<<10)-1));
  ledcWrite(MAGNET_LEDC_CH, raw);
}

void ledsOff(){
  for(int i=0;i<LED_COUNT;i++) pixels.SetPixelColor(i, RgbColor(0));
  pixels.Show();
}

void updateLEDs(float audioLevel01){
  if(!ledsEnabled || ledMode==LED_OFF){
    ledsOff();
    return;
  }

  // Faster envelope for visuals
  levelFast = (1.0f - lpfFast)*levelFast + lpfFast*(audioLevel01 * sensitivity);
  float L = clampf(levelFast, 0.f, 1.f);

  if(ledMode == LED_RAINBOW){
    rainbowOffset += rainbowBaseSpeed + rainbowGainSpeed * L;
    for(int i=0;i<LED_COUNT;i++){
      float t = fmodf(rainbowOffset + (float)i/LED_COUNT, 1.0f);
      RgbColor col = colorFromScalar(t);
      float b = clampf((0.15f + 0.85f*L) * ledBrightness, 0.f, 1.f);
      col = RgbColor(uint8_t(col.R*b), uint8_t(col.G*b), uint8_t(col.B*b));
      pixels.SetPixelColor(i, col);
    }
    pixels.Show();
  } else if(ledMode == LED_VU_METER){
    // simple peak/decay bar
    static float vu = 0.f;
    if(L > vu) vu = L;
    else       vu = clampf(vu - vuDecay, 0.f, 1.f);
    int lit = (int)(vu * LED_COUNT + 0.5f);
    for(int i=0;i<LED_COUNT;i++){
      bool on = i < lit;
      float s = on ? clampf((0.3f + 0.7f*L) * ledBrightness, 0.f, 1.f) : 0.f;
      RgbColor col = on ? colorFromScalar((float)i/LED_COUNT) : RgbColor(0);
      col = RgbColor(uint8_t(col.R*s), uint8_t(col.G*s), uint8_t(col.B*s));
      pixels.SetPixelColor(i, col);
    }
    pixels.Show();
  }
}

// ==== Buttons ====
void initBtn(Btn &b, int pin){
  b.pin = pin;
  if(pin >= 0){ pinMode(pin, INPUT_PULLUP); b.last = digitalRead(pin); }
  else { b.last = true; }
  b.lastMs = 0;
}
bool btnPressed(Btn &b){
  if(b.pin < 0) return false;
  bool now = digitalRead(b.pin);
  uint32_t ms = millis();
  if(!now && b.last && (ms - b.lastMs) > 50){ b.lastMs = ms; b.last = now; return true; }
  if(now != b.last){ b.last = now; b.lastMs = ms; }
  return false;
}
void handleButtons(){
  if(btnPressed(btns[0])){ float step=(magnetFreqHz<=MAGNET_FREQ_FINE_ZONE)?MAGNET_STEP_FINE:MAGNET_STEP_COARSE; setMagnetFreq(magnetFreqHz+step); Serial.printf("[KEY] Freq UP → %.2f Hz\n", magnetFreqHz); }
  if(btnPressed(btns[1])){ float step=(magnetFreqHz<=MAGNET_FREQ_FINE_ZONE)?MAGNET_STEP_FINE:MAGNET_STEP_COARSE; setMagnetFreq(magnetFreqHz-step); Serial.printf("[KEY] Freq DOWN → %.2f Hz\n", magnetFreqHz); }
  if(btnPressed(btns[2])){ magnetEnabled=!magnetEnabled; Serial.printf("[KEY] Magnet %s\n", magnetEnabled?"ENABLED":"DISABLED"); }
  if(btnPressed(btns[3])){ ledsEnabled=!ledsEnabled; Serial.printf("[KEY] LEDs %s\n", ledsEnabled?"ON":"OFF"); }
  if(btnPressed(btns[4])){ ledMode=(LedMode)(((int)ledMode+1)%3); Serial.printf("[KEY] LED Mode → %d\n", (int)ledMode); }
  if(btnPressed(btns[5])){ sensitivity=(sensitivity<1.25f)?1.5f:1.0f; Serial.printf("[KEY] Sensitivity → %.2f\n", sensitivity); }
}

// ==== Serial Commands ====
void printStatus(){
  Serial.printf(
    "[STAT] Freq=%.2f Hz | DutyMin=%.0f%% DutyMax=%.0f%% Base=%.0f%% | Sens=%.2f | LPFslow=%.3f LPFfast=%.3f | LED b=%.2f | Rain(spd=%.3f gain=%.3f) | VUdecay=%.3f | Mode=%d | Magnet=%s | LEDs=%s\n",
    magnetFreqHz, dutyMin*100, dutyMax*100, baseDutyOffset*100, sensitivity, lpfSlow, lpfFast,
    ledBrightness, rainbowBaseSpeed, rainbowGainSpeed, vuDecay, (int)ledMode,
    magnetEnabled?"ON":"OFF", ledsEnabled?"ON":"OFF"
  );
}

void printHelp(){
  Serial.println(
    "\n[SERIAL HELP]\n"
    "Freq:   '+'/'-' = fine/coarse up/down (≤8Hz=0.25, >8Hz=1.0)\n"
    "Magnet: 'm' toggle  | '['/']' dutyMin -/+ 2%  | '{'/'}' dutyMax -/+ 2%  | 'b'/'B' base duty -/+ 1%\n"
    "Sense:  'e'/'E' sensitivity -/+ 0.1  | 'u'/'U' lpfSlow -/+ 0.005  | 'y'/'Y' lpfFast -/+ 0.05\n"
    "LEDs:   'l' toggle  | '0' OFF, '1' RAINBOW, '2' VU  | 'g'/'G' brightness -/+ 0.05\n"
    "RB spd: 'r'/'R' rainbow base speed -/+ 0.001  | 't'/'T' rainbow gain speed -/+ 0.005\n"
    "VU:     'v'/'V' VU decay -/+ 0.01\n"
    "Misc:   '?' status now  | 'h' help  | 'p' toggle periodic status  | 'Z' reset defaults\n"
  );
}

void resetDefaults(){
  magnetFreqHz = 5.0f; setMagnetFreq(magnetFreqHz);
  dutyMin = 0.05f; dutyMax = 0.45f; baseDutyOffset = 0.00f;
  sensitivity = 1.0f;
  lpfSlow = 0.02f; lpfFast = 0.20f;
  ledBrightness = 0.60f;
  rainbowBaseSpeed = 0.005f; rainbowGainSpeed = 0.020f;
  vuDecay = 0.06f;
  Serial.println("[SER] Defaults restored.");
  printStatus();
}

void handleSerial(){
  while(Serial.available()){
    char c = Serial.read();
    switch(c){
      case '+': case '=':{ float s=(magnetFreqHz<=MAGNET_FREQ_FINE_ZONE)?MAGNET_STEP_FINE:MAGNET_STEP_COARSE; setMagnetFreq(magnetFreqHz+s); Serial.printf("[SER] Freq UP → %.2f Hz\n", magnetFreqHz);} break;
      case '-': case '_':{ float s=(magnetFreqHz<=MAGNET_FREQ_FINE_ZONE)?MAGNET_STEP_FINE:MAGNET_STEP_COARSE; setMagnetFreq(magnetFreqHz-s); Serial.printf("[SER] Freq DOWN → %.2f Hz\n", magnetFreqHz);} break;

      case 'm': magnetEnabled=!magnetEnabled; Serial.printf("[SER] Magnet %s\n", magnetEnabled?"ENABLED":"DISABLED"); break;
      case 'l': ledsEnabled=!ledsEnabled; Serial.printf("[SER] LEDs %s\n", ledsEnabled?"ON":"OFF"); break;
      case '0': case '1': case '2': { ledMode=(LedMode)(c-'0'); Serial.printf("[SER] LED Mode → %d\n", (int)ledMode);} break;

      // Duty window & base offset
      case '[': dutyMin = clampf(dutyMin - 0.02f, 0.00f, dutyMax - 0.02f); Serial.printf("[SER] DutyMin → %.0f%%\n", dutyMin*100); break;
      case ']': dutyMin = clampf(dutyMin + 0.02f, 0.00f, dutyMax - 0.02f); Serial.printf("[SER] DutyMin → %.0f%%\n", dutyMin*100); break;
      case '{': dutyMax = clampf(dutyMax - 0.02f, dutyMin + 0.02f, 0.90f); Serial.printf("[SER] DutyMax → %.0f%%\n", dutyMax*100); break;
      case '}': dutyMax = clampf(dutyMax + 0.02f, dutyMin + 0.02f, 0.90f); Serial.printf("[SER] DutyMax → %.0f%%\n", dutyMax*100); break;
      case 'b': baseDutyOffset = clampf(baseDutyOffset - 0.01f, -0.20f, 0.20f); Serial.printf("[SER] BaseDutyOffset → %.0f%%\n", baseDutyOffset*100); break;
      case 'B': baseDutyOffset = clampf(baseDutyOffset + 0.01f, -0.20f, 0.20f); Serial.printf("[SER] BaseDutyOffset → %.0f%%\n", baseDutyOffset*100); break;

      // Sensitivity & filters
      case 'e': sensitivity = clampf(sensitivity - 0.10f, 0.20f, 3.00f); Serial.printf("[SER] Sensitivity → %.2f\n", sensitivity); break;
      case 'E': sensitivity = clampf(sensitivity + 0.10f, 0.20f, 3.00f); Serial.printf("[SER] Sensitivity → %.2f\n", sensitivity); break;
      case 'u': lpfSlow = clampf(lpfSlow - 0.005f, 0.005f, 0.200f); Serial.printf("[SER] LPFslow → %.3f\n", lpfSlow); break;
      case 'U': lpfSlow = clampf(lpfSlow + 0.005f, 0.005f, 0.200f); Serial.printf("[SER] LPFslow → %.3f\n", lpfSlow); break;
      case 'y': lpfFast = clampf(lpfFast - 0.05f, 0.05f, 0.80f); Serial.printf("[SER] LPFfast → %.3f\n", lpfFast); break;
      case 'Y': lpfFast = clampf(lpfFast + 0.05f, 0.05f, 0.80f); Serial.printf("[SER] LPFfast → %.3f\n", lpfFast); break;

      // LED tuning
      case 'g': ledBrightness = clampf(ledBrightness - 0.05f, 0.05f, 1.00f); Serial.printf("[SER] LED Brightness → %.2f\n", ledBrightness); break;
      case 'G': ledBrightness = clampf(ledBrightness + 0.05f, 0.05f, 1.00f); Serial.printf("[SER] LED Brightness → %.2f\n", ledBrightness); break;
      case 'r': rainbowBaseSpeed = clampf(rainbowBaseSpeed - 0.001f, 0.000f, 0.050f); Serial.printf("[SER] Rainbow BaseSpeed → %.3f\n", rainbowBaseSpeed); break;
      case 'R': rainbowBaseSpeed = clampf(rainbowBaseSpeed + 0.001f, 0.000f, 0.050f); Serial.printf("[SER] Rainbow BaseSpeed → %.3f\n", rainbowBaseSpeed); break;
      case 't': rainbowGainSpeed = clampf(rainbowGainSpeed - 0.005f, 0.000f, 0.100f); Serial.printf("[SER] Rainbow GainSpeed → %.3f\n", rainbowGainSpeed); break;
      case 'T': rainbowGainSpeed = clampf(rainbowGainSpeed + 0.005f, 0.000f, 0.100f); Serial.printf("[SER] Rainbow GainSpeed → %.3f\n", rainbowGainSpeed); break;

      // VU meter decay
      case 'v': vuDecay = clampf(vuDecay - 0.01f, 0.01f, 0.20f); Serial.printf("[SER] VU Decay → %.3f\n", vuDecay); break;
      case 'V': vuDecay = clampf(vuDecay + 0.01f, 0.01f, 0.20f); Serial.printf("[SER] VU Decay → %.3f\n", vuDecay); break;

      // Status / help / telemetry / reset
      case '?': printStatus(); break;
      case 'h': printHelp(); break;
      case 'p': telemetry = !telemetry; Serial.printf("[SER] Telemetry %s\n", telemetry?"ON":"OFF"); break;
      case 'Z': resetDefaults(); break;
    }
  }
}

// ==== Setup & Loop ====
void setup(){
  Serial.begin(115200);
  delay(500);
  Serial.println("\nFerroWave — Tuning Panel Build");

  for(int i=0;i<6;i++){ btns[i].pin = -1; initBtn(btns[i], btns[i].pin); }

  pixels.Begin(); ledsOff();
  setupMagnetPWM();

  duplicator.add(i2s_out);
  duplicator.add(processing_stream);

  auto cfg = board.defaultConfig(TX_MODE);
  board.begin(cfg);

  a2dp_sink.start("FerroWave");
  Serial.println("Bluetooth A2DP sink started. Pair and play audio.");
  printHelp();
  printStatus();
  nextStatMs = millis() + statPeriodMs;
}

void loop(){
  handleButtons();
  handleSerial();

  size_t avail = processing_stream.available();
  if(avail >= SAMPLE_COUNT * sizeof(int16_t)){
    size_t got = processing_stream.readBytes((uint8_t*)sampleBuf, SAMPLE_COUNT * sizeof(int16_t));
    if(got == SAMPLE_COUNT * sizeof(int16_t)){
      double acc = 0.0;
      for(size_t i=0;i<SAMPLE_COUNT;i++){ float v = sampleBuf[i] / 32768.0f; acc += v*v; }
      float rms = sqrtf(acc / SAMPLE_COUNT);
      float level = clampf(rms * 2.0f, 0.0f, 1.0f);
      updateMagnetDuty(level);
      updateLEDs(level);
    }
  } else {
    updateMagnetDuty(0.0f);
    if(ledsEnabled && ledMode==LED_RAINBOW){
      rainbowOffset += rainbowBaseSpeed * 0.4f; // very slow idle
      for(int i=0;i<LED_COUNT;i++){
        float t = fmodf(rainbowOffset + (float)i/LED_COUNT, 1.0f);
        RgbColor col = colorFromScalar(t);
        float b = 0.1f * ledBrightness;
        col = RgbColor(uint8_t(col.R*b), uint8_t(col.G*b), uint8_t(col.B*b));
        pixels.SetPixelColor(i, col);
      }
      pixels.Show();
    }
    delay(2);
  }

  if(telemetry && millis() >= nextStatMs){
    printStatus();
    nextStatMs += statPeriodMs;
  }
}
