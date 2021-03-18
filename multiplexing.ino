/*
 * Arduino Clock Project
 * Author: Steve Rayner
 * Date: 23/01/2021
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

int segmentPins[7] = {2, 3, 4, 5, 6, 7, 8};
int digitPins[4] = {10, 11, 12, 13};
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

int hours = 17;
int minutes = 49;
int seconds = 0;

int speed = 2;

void setup() {
  for (int i = 0; i <= 6; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
  for(int i = 0; i <= 3; i++) {
    pinMode(digitPins[i], OUTPUT);
  }

  pinMode(A0, INPUT);
  
  // reset timer 1 control reg A
  TCCR1A = 0;

  // set prescaler of 256
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B &= ~(1 << CS10); 

  // reset timer 1 and set compare value
  TCNT1 = t1_load;
  OCR1A = t1_comp;

  TIMSK1 = (1 << OCIE1A);

  sei();
}

void loop() {
  int buttonState = digitalRead(A0);
  if (buttonState == HIGH) {
    incrementTime();
  }
  output(0, hours / 10);
  output(1, hours % 10);
  output(2, minutes / 10);
  output(3, minutes % 10);
}

void output(int digit, int value) {

  // Turn off all digits while we set the segments.
  for (int i = 0; i <= 3; i++) {
      digitalWrite(digitPins[i], HIGH);
  }
  
  // Set the required segments
  for (int i = 0; i <= 6 ; i++) {
    if (numbers[value][i] == 1) {
      digitalWrite(segmentPins[i], HIGH);
    } else {
      digitalWrite(segmentPins[i], LOW);
    }
  }

  // Turn on the required digit. 
  for (int i = 0; i <= 3; i++) {
    if (i == digit) {
      digitalWrite(digitPins[i], LOW);
    } else {
      digitalWrite(digitPins[i], HIGH);
    }
  }
  delay(1);
}

ISR(TIMER1_COMPA_vect) {

  // Reset timer.
  TCNT1 = t1_load;

  incrementTime();
}

void incrementTime() {

  // Increment time.
  seconds++;
  if (seconds > 59) {
    seconds = 0;
    minutes++;
    if (minutes > 59) {
      minutes = 0;
      hours++;
      if (hours > 23) {
        hours = 0;
      }
    }
  }
    
}
