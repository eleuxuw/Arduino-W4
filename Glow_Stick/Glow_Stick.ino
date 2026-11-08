const byte ORANGE_LED = 3;
const byte BLUE_LED = 5;
const byte BUTTON = 7;

const byte RGB_R = 9;
const byte RGB_G = 10;
const byte RGB_B = 11;

// RGB colors tuned to complement orange + blue
const byte COLORS[][3] = {
  {160,  90, 255}, // Electric Violet
  {255,  50, 140}, // Neon Magenta
  {  0, 207, 255}, // Arctic Cyan
  { 80,  50, 255}, // Royal Indigo
  {255, 100,  64}, // Sunset Coral
  {255, 215, 160}  // Warm White
};

const byte COLOR_COUNT =
  sizeof(COLORS) / sizeof(COLORS[0]);

byte colorIndex = 0;
byte mode = 0;

const unsigned long DEBOUNCE_MS = 35;
const unsigned long LONG_PRESS_MS = 800;
const unsigned long COLOR_STEP_MS = 2500;

int idleState;
int rawPrevious;
int stableState;

bool longHandled = false;

unsigned long lastChange = 0;
unsigned long pressStarted = 0;
unsigned long modeStarted = 0;
unsigned long cycleStarted = 0;

// Scale brightness (0–255)
byte scaleBrightness(byte value, byte level) {
  return (uint16_t)value * level / 255;
}

// Common-anode RGB uses inverted PWM
void setRGB(byte r, byte g, byte b) {
  analogWrite(RGB_R, 255 - r);
  analogWrite(RGB_G, 255 - g);
  analogWrite(RGB_B, 255 - b);
}

// Fixed-color LEDs
void setAccentLights(byte orangeLevel,
                     byte blueLevel) {
  analogWrite(ORANGE_LED, orangeLevel);
  analogWrite(BLUE_LED, blueLevel);
}

void showSelectedRGB(byte brightness) {
  setRGB(
    scaleBrightness(COLORS[colorIndex][0], brightness),
    scaleBrightness(COLORS[colorIndex][1], brightness),
    scaleBrightness(COLORS[colorIndex][2], brightness)
  );
}

void showAll(byte brightness) {
  setAccentLights(
    scaleBrightness(190, brightness),
    scaleBrightness(210, brightness)
  );

  showSelectedRGB(brightness);
}

// Linear interpolation between two PWM values
byte blendChannel(byte from, byte to,
                  unsigned long progress,
                  unsigned long duration) {
  long delta = (long)to - (long)from;
  long result = (long)from +
                delta * (long)progress / (long)duration;

  return (byte)constrain(result, 0L, 255L);
}

// Smoothly blend to the next curated RGB color
void showAurora() {
  unsigned long elapsed = millis() - cycleStarted;

  unsigned long stepNumber =
    elapsed / COLOR_STEP_MS;

  unsigned long progress =
    elapsed % COLOR_STEP_MS;

  byte startIndex =
    (colorIndex + stepNumber) % COLOR_COUNT;

  byte nextIndex =
    (startIndex + 1) % COLOR_COUNT;

  byte r = blendChannel(
    COLORS[startIndex][0],
    COLORS[nextIndex][0],
    progress, COLOR_STEP_MS
  );

  byte g = blendChannel(
    COLORS[startIndex][1],
    COLORS[nextIndex][1],
    progress, COLOR_STEP_MS
  );

  byte b = blendChannel(
    COLORS[startIndex][2],
    COLORS[nextIndex][2],
    progress, COLOR_STEP_MS
  );

  // Keep the two physical accent LEDs on,
  // but softer so the RGB transition stands out.
  setAccentLights(130, 160);
  setRGB(r, g, b);
}

void nextMode() {
  mode = (mode + 1) % 5;

  modeStarted = millis();
  cycleStarted = millis();

  Serial.print("MODE: ");
  Serial.println(mode + 1);
}

void nextColor() {
  colorIndex = (colorIndex + 1) % COLOR_COUNT;
  cycleStarted = millis();

  Serial.print("COLOR: ");
  Serial.println(colorIndex + 1);
}

void checkButton() {
  unsigned long now = millis();
  int reading = digitalRead(BUTTON);

  if (reading != rawPrevious) {
    lastChange = now;
  }

  if (now - lastChange >= DEBOUNCE_MS) {
    if (reading != stableState) {
      stableState = reading;

      if (stableState != idleState) {
        pressStarted = now;
        longHandled = false;
      } else {
        if (!longHandled) {
          nextColor();
        }
      }
    }
  }

  if (stableState != idleState &&
      !longHandled &&
      now - pressStarted >= LONG_PRESS_MS) {
    nextMode();
    longHandled = true;
  }

  rawPrevious = reading;
}

void updateLights() {
  unsigned long elapsed = millis() - modeStarted;

  switch (mode) {

    case 0:
      // 1. CONSTANT
      showAll(255);
      break;

    case 1:
      // 2. BLINK — 500 ms ON / OFF
      showAll((elapsed / 500UL) % 2 == 0 ? 255 : 0);
      break;

    case 2: {
      // 3. BREATHING — 2600 ms cycle
      unsigned long phase = elapsed % 2600UL;

      byte brightness;

      if (phase < 1300UL) {
        brightness = phase * 255UL / 1300UL;
      } else {
        brightness =
          (2600UL - phase) * 255UL / 1300UL;
      }

      showAll(brightness);
      break;
    }

    case 3:
      // 4. POLICE SIREN BONUS
      if ((elapsed / 350UL) % 2 == 0) {
        setAccentLights(255, 0);
        setRGB(255, 0, 0);
      } else {
        setAccentLights(0, 255);
        setRGB(0, 0, 255);
      }
      break;

    case 4:
      // 5. AURORA COLOR CYCLE BONUS
      showAurora();
      break;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(ORANGE_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);

  // Your powered three-pin button module
  pinMode(BUTTON, INPUT);

  setAccentLights(0, 0);
  setRGB(0, 0, 0);

  // Button must be unpressed at startup.
  delay(200);

  idleState = digitalRead(BUTTON);
  rawPrevious = idleState;
  stableState = idleState;

  modeStarted = millis();
  cycleStarted = millis();

  Serial.println("AURORA PLASMA READY");
  Serial.println("Short press = RGB color");
  Serial.println("Long press = lighting mode");
}

void loop() {
  checkButton();
  updateLights();
}
