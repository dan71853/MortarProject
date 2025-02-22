
const uint8_t DELAY_POT = 36;
const uint8_t POWER_POT = 39;
const uint8_t ARM_BTN = 32;
const uint8_t STATUS_LED = 2;  //35;

const uint8_t LIMIT_SW = 27;
const uint8_t RELAY = 14;


//Callbacks for external interrupts
void IRAM_ATTR armBtnCb();
void IRAM_ATTR limitSwCb();

//Global variables
bool armedState = false;  //True if motar is armed

enum States {
  DISARMED,
  ARMED,
  TRIGGERED,
  FIRING
};

char* stateNames[]= {
  "DISARMED",
  "ARMED",
  "TRIGGERED",
  "FIRING"
};
enum States currentState = DISARMED;

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
  attachInterrupt(digitalPinToInterrupt(LIMIT_SW), limitSwCb, RISING);

  //Setup Armed LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, armedState);
}

uint32_t debounceTimer = millis();

void IRAM_ATTR armBtnCb() {
  if (millis() - debounceTimer > 500) {
    if (currentState == DISARMED) currentState = ARMED;
    else currentState = DISARMED;
    digitalWrite(STATUS_LED, armedState);
    debounceTimer = millis();
  }
}

void IRAM_ATTR limitSwCb() {
}

enum States lastState;
void loop() {
  if (lastState != currentState) {
    Serial.printf("State: %s\n", stateNames[currentState]);
    lastState = currentState;
  }

  switch (currentState) {
    case DISARMED:
      break;
    case ARMED:
      break;
    case TRIGGERED:
      break;
    case FIRING:
      break;
    default:
      break;
  }
}
