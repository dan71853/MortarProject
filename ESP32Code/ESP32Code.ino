
const uint8_t DELAY_POT = 36;
const uint8_t POWER_POT = 39;
const uint8_t ARM_BTN = 32;
const uint8_t STATUS_LED = 2;  //35;

const uint8_t LIMIT_SW = 27;
const uint8_t RELAY = 14;

const bool LIMIT_SWITCH_FIRED = LOW;  //State of limit switch when it is triggered

//Callbacks for external interrupts
void IRAM_ATTR armBtnCb();
void IRAM_ATTR limitSwCb();

//Global variables
uint16_t triggeredTimeThreshold = 1000;
uint16_t fireTimeThreshold = 1000;

enum States {
  DISARMED,
  ARMED,
  TRIGGERED,
  FIRING,
  PRELOAD
};

char* stateNames[] = {
  "DISARMED",
  "ARMED",
  "TRIGGERED",
  "FIRING",
  "PRELOAD",
};
volatile enum States currentState = DISARMED;

//Delays
uint16_t initialDelay = 0;
uint16_t powerDelay = 0;


void setup() {
  Serial.begin(115200);

  //Setup arm button with interrupt
  pinMode(ARM_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ARM_BTN), armBtnCb, RISING);

  //Setup limit switch with interrupt
  pinMode(LIMIT_SW, INPUT_PULLUP);

  //Setup Armed LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
}

uint32_t armDebounceTimer = millis();
void IRAM_ATTR armBtnCb() {
  if (millis() - armDebounceTimer > 500) {
    if (currentState == DISARMED) currentState = ARMED;
    else currentState = DISARMED;
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
      //Run once
      if (lastState != currentState) {
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
