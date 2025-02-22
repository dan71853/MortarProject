// Pins
const uint8_t DELAY_POT = 36;   // Potentiometer to control the initial delay
const uint8_t POWER_POT = 39;   // Potentiometer to control how long relay/solenoid is open for
const uint8_t ARM_BTN = 32;     // Momentary illuminated push button to arm/disarm, pull up pin internally
const uint8_t STATUS_LED = 33;  // LED in the push button (can be powered directly from pin)

const uint8_t LIMIT_SW = 27; //Limit switch to detect mortar shell
const uint8_t RELAY = 2;  // 14

const bool RELAY_ON_STATE = HIGH;     // Relay state to fire motor
const bool LIMIT_SWITCH_FIRED = LOW;  // State of limit switch when it is triggered

// Callbacks for external interrupts
void IRAM_ATTR armBtnCb();
void IRAM_ATTR limitSwCb();

// Global variables
uint16_t triggeredTimeThreshold = 1000;
const uint16_t TRIGGERED_TIME_MIN = 100;
const uint16_t TRIGGERED_TIME_MAX = 1000;

uint16_t fireTimeThreshold = 1000;
const uint16_t FIRE_TIME_MIN = 100;
const uint16_t FIRE_TIME_MAX = 1000;

enum States {
  DISARMED,
  ARMED,
  TRIGGERED,
  FIRING,
  PRELOAD
};

char *stateNames[] = {
  "DISARMED",
  "ARMED",
  "TRIGGERED",
  "FIRING",
  "PRELOAD",
};
volatile enum States currentState = DISARMED;

// Delays
uint16_t initialDelay = 0;
uint16_t powerDelay = 0;

void setup() {
  Serial.begin(115200);

  // Setup relay output
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, !RELAY_ON_STATE);

  // Setup arm button with interrupt
  pinMode(ARM_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ARM_BTN), armBtnCb, RISING);

  // Setup limit switch with interrupt
  pinMode(LIMIT_SW, INPUT_PULLUP);

  // Setup Armed LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  // Setup analog inputs
  analogReadResolution(10);
  pinMode(DELAY_POT, INPUT);
  pinMode(POWER_POT, INPUT);
}

uint32_t armDebounceTimer = millis();
void IRAM_ATTR armBtnCb() {
  if (millis() - armDebounceTimer > 500) {
    if (currentState == DISARMED)
      currentState = ARMED;
    else
      currentState = DISARMED;
    armDebounceTimer = millis();
  }
}

enum States lastPrintState;
enum States lastState;
uint32_t triggeredTimer = millis();
uint32_t fireTimer = millis();
void loop() {

  switch (currentState) {
    case DISARMED:
      // Run once
      if (lastState != currentState) {
        digitalWrite(RELAY, !RELAY_ON_STATE);
        lastState = currentState;
      }

      if (digitalRead(LIMIT_SW) == LIMIT_SWITCH_FIRED) {
        currentState = PRELOAD;
        break;
      }
      digitalWrite(STATUS_LED, LOW);
      break;

    case ARMED:
      if (lastState != currentState) {
      }
      if (digitalRead(LIMIT_SW) == LIMIT_SWITCH_FIRED) {
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
        triggeredTimeThreshold = map(potValue, 0, 1023, TRIGGERED_TIME_MIN, TRIGGERED_TIME_MAX);
        Serial.printf("pot: %d, time: %d\n", potValue, triggeredTimeThreshold);
      }
      if (millis() - triggeredTimer > triggeredTimeThreshold) {
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
        digitalWrite(RELAY, RELAY_ON_STATE);
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
      if (digitalRead(LIMIT_SW) != LIMIT_SWITCH_FIRED) {
        currentState = DISARMED;
      }
      break;

    default:
      break;
  }

  if (lastPrintState != currentState) {
    Serial.printf("State: %s\n", stateNames[currentState]);
    lastPrintState = currentState;
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
