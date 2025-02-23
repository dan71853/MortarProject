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
const uint16_t FIRE_TIME_MAX = 5000;

//Analog resoloution
const uint8_t ANALOG_RES = 10;

// State machine states
enum States {
  DISARMED,
  ARMED,
  TRIGGERED_SETUP,
  TRIGGERED,
  FIRING_SETUP,
  FIRING,
  PRELOAD
};
volatile enum States currentState = DISARMED;

// States in string format for printing
char *stateNames[] = {
  "DISARMED",
  "ARMED",
  "TRIGGERED_SETUP",
  "TRIGGERED",
  "FIRING_SETUP",
  "FIRING",
  "PRELOAD",
};

enum LedMode {
  LED_OFF,         //Disarmed
  LED_ON,          //Armed
  LED_BLINK_FAST,  //Firing
  LED_BLINK_SLOW   //Error
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
  analogReadResolution(ANALOG_RES);
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

  if (lastState != currentState) {  //used for debugging
    Serial.printf("State: %s\n", stateNames[currentState]);
  }

  switch (currentState) {

    case DISARMED:
      digitalWrite(SOLENOID_PIN, !SOLENOID_ON_STATE);  // Force solenoid off

      if (digitalRead(LIMIT_SW) == LIMIT_SWITCH_ACTIVATED) {  //If limit switch is triggered then go to the error state PRELOAD
        currentState = PRELOAD;
        return;
      }
      setLED(LED_OFF);
      break;

    case ARMED:
      digitalWrite(SOLENOID_PIN, !SOLENOID_ON_STATE);         // Force solenoid off
      if (digitalRead(LIMIT_SW) == LIMIT_SWITCH_ACTIVATED) {  //If limit switch is triggered then start the firing procudure
        currentState = TRIGGERED_SETUP;
        return;
      }
      setLED(LED_ON);
      break;

    case TRIGGERED_SETUP:                                                                                                   //This is run once before going to triggered state
      triggeredTimer = millis();                                                                                            //reset the timer
      triggerDelayThreshold = map(analogRead(DELAY_POT), 0, pow(2, ANALOG_RES) - 1, TRIGGER_DELAY_MIN, TRIGGER_DELAY_MAX);  //Map the analog reading to the time delay
      currentState = TRIGGERED;
      return;

    case TRIGGERED:
      if (millis() - triggeredTimer > triggerDelayThreshold) {  //Wait for the initial delay time
        currentState = FIRING_SETUP;
        return;
      }
      setLED(LED_BLINK_FAST);
      break;

    case FIRING_SETUP:  //Open the solenoid and restart the timer
      fireTimer = millis();
      fireTimeThreshold = map(analogRead(POWER_POT), 0, pow(2, ANALOG_RES) - 1, FIRE_TIME_MIN, FIRE_TIME_MAX);  //Map the analog reading to the time delay
      Serial.println(fireTimeThreshold);
      digitalWrite(SOLENOID_PIN, SOLENOID_ON_STATE);
      currentState = FIRING;
      return;

    case FIRING:  //Wait for the motor to fire
      if (millis() - fireTimer > fireTimeThreshold) {
        currentState = DISARMED;
        return;
      }
      setLED(LED_BLINK_FAST);
      break;

    case PRELOAD:              //Error state where the limit switch has been activated when disarmed
      setLED(LED_BLINK_SLOW);  //Blink the light slowly and wait for the limit switch to be cleared
      if (digitalRead(LIMIT_SW) != LIMIT_SWITCH_ACTIVATED) {
        currentState = DISARMED;
        return;
      }
      break;

    default:
      break;
  }

  lastState = currentState;
}

uint32_t blinkTimer = millis();
uint16_t blinkThreshold = 100;
bool ledState = false;
/**
 * Set the status LED
 */
void setLED(LedMode ledMode) {
  switch (ledMode) {
    case LED_OFF:
      ledState = LOW;
      break;

    case LED_ON:
      ledState = HIGH;
      break;

    case LED_BLINK_FAST:
      blinkThreshold = 100;
      if (millis() - blinkTimer > blinkThreshold) {
        blinkTimer = millis();
        ledState = !ledState;
        digitalWrite(STATUS_LED, ledState);
      }
      break;

    case LED_BLINK_SLOW:
      blinkThreshold = 500;
      if (millis() - blinkTimer > blinkThreshold) {
        blinkTimer = millis();
        ledState = !ledState;
      }
      break;

    default:
      Serial.println("Error: Invalid LED mode");
      break;
  }
  digitalWrite(STATUS_LED, ledState);
}
