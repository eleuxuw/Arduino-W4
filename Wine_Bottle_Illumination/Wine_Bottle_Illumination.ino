// ============================================================
// RGB CONTROLLER - SMOOTH PWM VERSION
//
// Single click         = Next preset color
// Hold                 = Smooth rainbow
// Double click         = LED ON/OFF
// Double click + hold  = Disco
//
// ACTIVE LOW RGB MODULE
//
// WIRING:
// Button -> D2
// Red    -> D3
// Green  -> D6
// Blue   -> D5
// ============================================================


// ---------------- PINS ----------------

const byte BUTTON_PIN = 2;

const byte RED_PIN   = 3;
const byte GREEN_PIN = 6;
const byte BLUE_PIN  = 5;


// ---------------- BUTTON TIMING ----------------

const unsigned long DEBOUNCE_MS     = 25;
const unsigned long DOUBLE_CLICK_MS = 350;
const unsigned long HOLD_MS         = 500;


// ---------------- ANIMATION ----------------

const unsigned long RAINBOW_FRAME_MS = 12;

// Smaller = slower rainbow
const float RAINBOW_SPEED = 0.7;


// ============================================================
// BUTTON VARIABLES
// ============================================================

bool lastRawButton = HIGH;
bool stableButton  = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;
unsigned long firstReleaseTime = 0;

bool waitingForSecondClick = false;
bool secondPress = false;

bool rainbowMode = false;
bool discoMode = false;


// ============================================================
// LED VARIABLES
// ============================================================

bool ledEnabled = true;

byte presetIndex = 0;

float rainbowHue = 0;

unsigned long lastRainbowFrame = 0;
unsigned long lastDiscoFrame = 0;


// ============================================================
// SETUP
// ============================================================

void setup() {

  pinMode(BUTTON_PIN, INPUT);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  Serial.begin(9600);

  randomSeed(analogRead(A0));

  showPreset();

  Serial.println("RGB controller ready.");
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long now = millis();

  readButton(now);


  // ----------------------------------------------------------
  // RAINBOW
  // ----------------------------------------------------------

  if (rainbowMode && ledEnabled) {

    if (now - lastRainbowFrame >= RAINBOW_FRAME_MS) {

      lastRainbowFrame = now;

      rainbowHue += RAINBOW_SPEED;

      if (rainbowHue >= 360.0) {
        rainbowHue -= 360.0;
      }

      showHue(rainbowHue);
    }
  }


  // ----------------------------------------------------------
  // DISCO
  // ----------------------------------------------------------

  if (discoMode && ledEnabled) {

    if (now - lastDiscoFrame >= 80) {

      lastDiscoFrame = now;

      showDiscoColor();
    }
  }


  // ----------------------------------------------------------
  // CONFIRM SINGLE CLICK
  // ----------------------------------------------------------

  if (
    waitingForSecondClick &&
    stableButton == HIGH &&
    now - firstReleaseTime > DOUBLE_CLICK_MS
  ) {

    waitingForSecondClick = false;

    nextPreset();
  }
}


// ============================================================
// BUTTON HANDLING
// ============================================================

void readButton(unsigned long now) {

  bool reading = digitalRead(BUTTON_PIN);


  // ----------------------------------------------------------
  // DEBOUNCE
  // ----------------------------------------------------------

  if (reading != lastRawButton) {

    lastDebounceTime = now;
    lastRawButton = reading;
  }


  if (now - lastDebounceTime < DEBOUNCE_MS) {
    return;
  }


  if (reading == stableButton) {
    checkHold(now);
    return;
  }


  stableButton = reading;


  // ==========================================================
  // BUTTON PRESSED
  // ==========================================================

  if (stableButton == LOW) {

    pressStartTime = now;


    // Is this the second press?
    if (
      waitingForSecondClick &&
      now - firstReleaseTime <= DOUBLE_CLICK_MS
    ) {

      secondPress = true;
    }

    else {

      secondPress = false;
    }
  }


  // ==========================================================
  // BUTTON RELEASED
  // ==========================================================

  else {

    // --------------------------------------------------------
    // RELEASE FROM DISCO
    // --------------------------------------------------------

    if (discoMode) {

      discoMode = false;
      secondPress = false;
      waitingForSecondClick = false;

      Serial.println("DISCO STOP");

      // Leave current disco color frozen
      return;
    }


    // --------------------------------------------------------
    // RELEASE FROM RAINBOW
    // --------------------------------------------------------

    if (rainbowMode) {

      rainbowMode = false;
      secondPress = false;
      waitingForSecondClick = false;

      Serial.println("RAINBOW STOP");

      // IMPORTANT:
      // Do NOT turn LED off.
      // Do NOT restore preset.
      //
      // Current mixed RGB color stays visible.

      return;
    }


    // --------------------------------------------------------
    // SECOND CLICK RELEASE
    // --------------------------------------------------------

    if (secondPress) {

      secondPress = false;
      waitingForSecondClick = false;

      ledEnabled = !ledEnabled;


      if (ledEnabled) {

        Serial.println("LED ON");

        showPreset();
      }

      else {

        Serial.println("LED OFF");

        setRGB(0, 0, 0);
      }

      return;
    }


    // --------------------------------------------------------
    // FIRST CLICK RELEASE
    // --------------------------------------------------------

    waitingForSecondClick = true;
    firstReleaseTime = now;
  }
}


// ============================================================
// HOLD DETECTION
// ============================================================

void checkHold(unsigned long now) {

  if (stableButton != LOW) {
    return;
  }


  unsigned long held =
    now - pressStartTime;


  if (held < HOLD_MS) {
    return;
  }


  // ==========================================================
  // SECOND PRESS HELD = DISCO
  // ==========================================================

  if (secondPress) {

    if (!discoMode) {

      discoMode = true;
      rainbowMode = false;

      Serial.println("DISCO START");
    }

    return;
  }


  // ==========================================================
  // FIRST PRESS HELD = RAINBOW
  // ==========================================================

  if (!rainbowMode) {

    rainbowMode = true;
    discoMode = false;

    waitingForSecondClick = false;

    Serial.println("RAINBOW START");
  }
}


// ============================================================
// NEXT PRESET
// ============================================================

void nextPreset() {

  if (!ledEnabled) {
    return;
  }

  presetIndex++;

  if (presetIndex > 6) {
    presetIndex = 0;
  }

  showPreset();
}


// ============================================================
// PRESET COLORS
// ============================================================

void showPreset() {

  if (!ledEnabled) {

    setRGB(0, 0, 0);
    return;
  }


  switch (presetIndex) {

    // RED
    case 0:

      setRGB(255, 0, 0);
      rainbowHue = 0;

      Serial.println("RED");
      break;


    // ORANGE
    case 1:

      setRGB(255, 70, 0);
      rainbowHue = 16;

      Serial.println("ORANGE");
      break;


    // YELLOW
    case 2:

      setRGB(255, 255, 0);
      rainbowHue = 60;

      Serial.println("YELLOW");
      break;


    // GREEN
    case 3:

      setRGB(0, 255, 0);
      rainbowHue = 120;

      Serial.println("GREEN");
      break;


    // CYAN
    case 4:

      setRGB(0, 255, 255);
      rainbowHue = 180;

      Serial.println("CYAN");
      break;


    // BLUE
    case 5:

      setRGB(0, 0, 255);
      rainbowHue = 240;

      Serial.println("BLUE");
      break;


    // PURPLE
    case 6:

      setRGB(255, 0, 255);
      rainbowHue = 300;

      Serial.println("PURPLE");
      break;
  }
}


// ============================================================
// SMOOTH HSV-LIKE RAINBOW
//
// There is NEVER an OFF step between colors.
//
// Example:
// RED
// 255,0,0
//
// becomes
// 255,1,0
// 255,2,0
// 255,3,0
// ...
// 255,255,0
//
// = smooth red -> orange -> yellow
// ============================================================

void showHue(float hue) {

  float x;

  int r = 0;
  int g = 0;
  int b = 0;


  // RED -> YELLOW
  if (hue < 60) {

    x = hue / 60.0;

    r = 255;
    g = 255 * x;
    b = 0;
  }


  // YELLOW -> GREEN
  else if (hue < 120) {

    x = (hue - 60.0) / 60.0;

    r = 255 * (1.0 - x);
    g = 255;
    b = 0;
  }


  // GREEN -> CYAN
  else if (hue < 180) {

    x = (hue - 120.0) / 60.0;

    r = 0;
    g = 255;
    b = 255 * x;
  }


  // CYAN -> BLUE
  else if (hue < 240) {

    x = (hue - 180.0) / 60.0;

    r = 0;
    g = 255 * (1.0 - x);
    b = 255;
  }


  // BLUE -> MAGENTA
  else if (hue < 300) {

    x = (hue - 240.0) / 60.0;

    r = 255 * x;
    g = 0;
    b = 255;
  }


  // MAGENTA -> RED
  else {

    x = (hue - 300.0) / 60.0;

    r = 255;
    g = 0;
    b = 255 * (1.0 - x);
  }


  setRGB(r, g, b);
}


// ============================================================
// DISCO
//
// Random BRIGHT mixed colors.
// ============================================================

void showDiscoColor() {

  int choice = random(0, 10);


  switch (choice) {

    case 0:
      setRGB(255, 0, 0);       // Red
      break;

    case 1:
      setRGB(255, 60, 0);      // Orange
      break;

    case 2:
      setRGB(255, 180, 0);     // Gold
      break;

    case 3:
      setRGB(255, 255, 0);     // Yellow
      break;

    case 4:
      setRGB(0, 255, 0);       // Green
      break;

    case 5:
      setRGB(0, 255, 255);     // Cyan
      break;

    case 6:
      setRGB(0, 80, 255);      // Azure
      break;

    case 7:
      setRGB(0, 0, 255);       // Blue
      break;

    case 8:
      setRGB(180, 0, 255);     // Purple
      break;

    case 9:
      setRGB(255, 0, 100);     // Pink
      break;
  }
}


// ============================================================
// RGB OUTPUT
//
// THIS IS THE IMPORTANT PART.
//
// Input:
//
//   255 = fully ON
//   0   = fully OFF
//
// Your RGB module is ACTIVE LOW,
// so PWM values are inverted before writing.
//
// NO turn-off between colors.
// ============================================================

void setRGB(int r, int g, int b) {

  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);


  analogWrite(
    RED_PIN,
    255 - r
  );

  analogWrite(
    GREEN_PIN,
    255 - g
  );

  analogWrite(
    BLUE_PIN,
    255 - b
  );
}