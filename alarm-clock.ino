/*
 * Arduino Clock Project
 * Author: Steve Rayner
 * Date: 23/01/2021 - 09/04/2025
 * 
 *      A
 *     ---
 *  F |   | B
 *    | G |
 *     ---
 *  E |   | C
 *    |   |
 *     ---
 *      D
 */

// counter and compare values
const uint16_t t1_load = 0;
const uint16_t t1_comp = 62500;

// define input pins
const int BTN_COUNT = 5;
int inputPins[BTN_COUNT] = {14, 15, 16, 17, 18};

// define output pins
int segmentPins[7] = {2, 3, 4, 5, 6, 7, 8};
int digitPins[4] = {10, 11, 12, 13};

// 7-segment encoded values
int numbers[10][7] = {
  {1, 1, 1, 1, 1, 1, 0},
  {0, 1, 1, 0, 0, 0, 0},
  {1, 1, 0, 1, 1, 0, 1},
  {1, 1, 1, 1, 0, 0, 1},
  {0, 1, 1, 0, 0, 1, 1},
  {1, 0, 1, 1, 0, 1, 1},
  {0, 0, 1, 1, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 0, 0, 1, 1},
};

// button handling constants
const unsigned long debounceDelay = 30;       // ms
const unsigned long initialRepeatDelay = 500; // ms
const unsigned long repeatRate = 200;         // ms

enum Button {
  BTN_TIME = 0,
  BTN_ALARM,
  BTN_HOUR,
  BTN_MINUTE,
  BTN_SNOOZE
};

int timeHours = 12, timeMinutes = 0, timeSeconds = 0;
int alarmHours = 7, alarmMinutes = 30;

struct ButtonState {
  bool stableState = false;         // debounced state
  bool lastStable = false;          // last loop's stable state
  unsigned long lastChangeTime = 0;
};

ButtonState buttonStates[BTN_COUNT];

unsigned long lastRepeatTime = 0;
int activeCombo = -1;

void setup() {

  // set up the input pins
  for (int i = 0; i <= 4; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
  }

  // setup the output pins
  for (int i = 0; i <= 6; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
  for(int i = 0; i <= 3; i++) {
    pinMode(digitPins[i], OUTPUT);
  }
  
  // reset timer 1 control reg A
  TCCR1A = 0;

  // set prescaler of 256
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B &= ~(1 << CS10);

  // reset timer 1 and set compare value
  TCNT1 = t1_load;
  OCR1A = t1_comp;

  // output compare match A interrupt enable
  TIMSK1 = (1 << OCIE1A);

  // set enable interupt flag
  sei();
}

void loop() {

  unsigned long now = millis();

  // 1. Read Buttons with Debounce
  for (int i = 0; i < BTN_COUNT; i++) {
    bool rawState = digitalRead(inputPins[i]) == LOW;
    if (rawState != buttonStates[i].stableState) {
      if (now - buttonStates[i].lastChangeTime > debounceDelay) {
        buttonStates[i].lastStable = buttonStates[i].stableState;
        buttonStates[i].stableState = rawState;
        buttonStates[i].lastChangeTime = now;
      }
    } else {
      buttonStates[i].lastChangeTime = now;
    }
  }

  // 2. Detect Active Combo
  int newCombo = detectCombo();

  // 3. Handle Combo Action with Auto-Repeat
  if (newCombo != -1) {
    if (newCombo != activeCombo) {
      // New combo started
      activeCombo = newCombo;
      lastRepeatTime = now;
      performComboAction(newCombo);
    } else if (now - lastRepeatTime >= repeatRate) {
      // Hold repeating
      lastRepeatTime = now;
      performComboAction(newCombo);
    }
  } else {
    activeCombo = -1; // No active combo
  }

  // 4. Display Time or Alarm
  if (buttonStates[BTN_ALARM].stableState) {
    displayTime(alarmHours, alarmMinutes);
  } else {
    displayTime(timeHours, timeMinutes);
  }
}

/*
 * Detect button combination
 */
int detectCombo() {
  bool time = buttonStates[BTN_TIME].stableState;
  bool alarm = buttonStates[BTN_ALARM].stableState;
  bool hour = buttonStates[BTN_HOUR].stableState;
  bool minute = buttonStates[BTN_MINUTE].stableState;

  if (time && hour) return 0;   // Advance time hour
  if (time && minute) return 1; // Advance time minute
  if (alarm && hour) return 2;  // Advance alarm hour
  if (alarm && minute) return 3;// Advance alarm minute

  return -1;
}

/*
 * Perform action based upon button combination
 */
void performComboAction(int combo) {
  switch (combo) {
    case 0:
      timeHours = (timeHours + 1) % 24;
      break;
    case 1:
      timeMinutes = (timeMinutes + 1) % 60;
      break;
    case 2:
      alarmHours = (alarmHours + 1) % 24;
      break;
    case 3:
      alarmMinutes = (alarmMinutes + 1) % 60;
      break;
  }
}

/*
 * Displays a time
 */
void displayTime(int hours, int minutes) {
  output(0, (hours / 10) % 10);
  output(1, hours % 10);
  output(2, (minutes / 10) % 10);
  output(3, minutes % 10);
}

void output(int digit, int value) {

  // Turn off all digits while we set the segments.
  for (int i = 0; i <= 3; i++) {
      digitalWrite(digitPins[i], LOW);
  }
  
  // Set the required segments
  for (int i = 0; i <= 6 ; i++) {
    if (numbers[value][i] == 1) {
      digitalWrite(segmentPins[i], LOW);
    } else {
      digitalWrite(segmentPins[i], HIGH);
    }
  }

  // Turn on the required digit. 
  digitalWrite(digitPins[digit], HIGH);
    
  delay(1);
}

ISR(TIMER1_COMPA_vect) {

  // Reset timer.
  TCNT1 = t1_load;

  incrementTime();
}

void incrementTime() {
  timeSeconds = (timeSeconds + 1) % 60;
  if (timeSeconds == 0) {
    timeMinutes = (timeMinutes + 1) % 60;
    if (timeMinutes == 0) {
      timeHours = (timeHours + 1) % 24;
    }
  }
}
