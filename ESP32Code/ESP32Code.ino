// Pins
const uint8_t DELAY_POT = 36;   // Potentiometer to control the initial delay
const uint8_t POWER_POT = 39;   // Potentiometer to control how long relay/solenoid is open for
const uint8_t ARM_BTN = 32;     // Momentary illuminated push button to arm/disarm, pull up pin internally
const uint8_t STATUS_LED = 33;  // LED in the push button (can be powered directly from pin)

const uint8_t LIMIT_SW = 27;     // Limit switch to detect mortar shell
const uint8_t SOLENOID_PIN = 2;  // 14 set to onboard led (2) for testing //Relay or MOSFET to control solenoid

// Define ON/OFF state for inputs/outputs
const bool SOLENOID_ON_STATE = HIGH;      // true = Activate solenoid
const bool LIMIT_SWITCH_ACTIVATED = LOW;  // true = Mortar shell detected by limit switch

// Callbacks for external interrupts
void IRAM_ATTR armBtnCb();

// Global variables for timing, time in ms
uint16_t triggerDelayThreshold = 1000;  // Initial delay
const uint16_t TRIGGER_DELAY_MIN = 100;
const uint16_t TRIGGER_DELAY_MAX = 1000;

uint16_t fireTimeThreshold = 1000;  // Time solenoid is open for
const uint16_t FIRE_TIME_MIN = 100;
const uint16_t FIRE_TIME_MAX = 1000;

// State machine states
enum States {
  DISARMED,
  ARMED,
  TRIGGERED,
  FIRING,
  PRELOAD
};
volatile enum States currentState = DISARMED;

// States in string format for printing
char *stateNames[] = {
  "DISARMED",
  "ARMED",
  "TRIGGERED",
  "FIRING",
  "PRELOAD",
};

void setup() {
  Serial.begin(115200);

  // Setup solenoid output and turn it off
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, !SOLENOID_ON_STATE);

  // Setup arm button with interrupt
  pinMode(ARM_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ARM_BTN), armBtnCb, RISING);

  // Setup limit switch, no interrupt nedded
  pinMode(LIMIT_SW, INPUT_PULLUP);

  // Setup Armed LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  // Setup analog inputs, set the resoloution to 10Bits (0-1024)
  analogReadResolution(10);
  pinMode(DELAY_POT, INPUT);
  pinMode(POWER_POT, INPUT);
}

uint32_t armDebounceTimer = millis();
const uint16_t debounceThreshold = 700;
/**
 * Callback for the rising edge of the pushbutton
 * Debouncing time based of debounceThreshold
 */
void IRAM_ATTR armBtnCb() {
  if (millis() - armDebounceTimer > debounceThreshold) {
    // Arm button will override all other states
    currentState = (currentState == DISARMED) ? ARMED : DISARMED;
    armDebounceTimer = millis();
  }
}

enum States lastState;
uint32_t triggeredTimer = millis();
uint32_t fireTimer = millis();
void loop() {

  if (lastState != currentState) {
    Serial.printf("State: %s\n", stateNames[currentState]);
  }

  switch (currentState) {
    case DISARMED:
      if (lastState != currentState) {
        // Run this part only once
        lastState = currentState;
      }
      digitalWrite(SOLENOID_PIN, !SOLENOID_ON_STATE);

      if (digitalRead(LIMIT_SW) == LIMIT_SWITCH_ACTIVATED) {
        currentState = PRELOAD;
        break;
      }
      digitalWrite(STATUS_LED, LOW);
      break;

    case ARMED:
      if (lastState != currentState) {
        lastState = currentState;
      }
      if (digitalRead(LIMIT_SW) == LIMIT_SWITCH_ACTIVATED) {
        currentState = TRIGGERED;
        break;
      }
      digitalWrite(STATUS_LED, HIGH);
      break;

    case TRIGGERED:
      if (lastState != currentState) {
        lastState = currentState;
        triggeredTimer = millis();
        uint16_t potValue = analogRead(DELAY_POT);
        triggerDelayThreshold = map(potValue, 0, 1023, TRIGGER_DELAY_MIN, TRIGGER_DELAY_MAX);
        Serial.printf("pot: %d, time: %d\n", potValue, triggerDelayThreshold);
      }
      if (millis() - triggeredTimer > triggerDelayThreshold) {
        currentState = FIRING;
      }
      blinkLEDFast();
      break;

    case FIRING:
      if (lastState != currentState) {
        lastState = currentState;
        fireTimer = millis();
        uint16_t potValue = analogRead(POWER_POT);
        fireTimeThreshold = map(potValue, 0, 1023, FIRE_TIME_MIN, FIRE_TIME_MAX);
        digitalWrite(SOLENOID_PIN, SOLENOID_ON_STATE);
        Serial.printf("pot: %d, time: %d\n", potValue, fireTimeThreshold);
      }
      if (millis() - fireTimer > fireTimeThreshold) {
        currentState = DISARMED;
      }
      blinkLEDFast();
      break;

    case PRELOAD:
      if (lastState != currentState) {
        lastState = currentState;
      }
      blinkLEDFast();
      if (digitalRead(LIMIT_SW) != LIMIT_SWITCH_ACTIVATED) {
        currentState = DISARMED;
      }
      break;

    default:
      break;
  }
}

uint32_t blinkTimer = millis();
const uint16_t blinkThreshold = 100;
bool blinkState = false;
/**
 * Blink the STATUS_LED quickly
 * Used to show when the limit switch has been triggered
 */
void blinkLEDFast() {
  if (millis() - blinkTimer > blinkThreshold) {
    blinkTimer = millis();
    blinkState = !blinkState;
    digitalWrite(STATUS_LED, blinkState);
  }
}
