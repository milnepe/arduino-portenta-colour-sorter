/*
  Portenta H7 Colour Sorter for M4 Core

  M4 core is responsible for controlling the stepper motor via
  an EasyDriver controller. The carousel continuously rotates and only
  stops momentarily to receive a counter.

  M4 colour index is updated by M7 via RPC each time it identifies the
  colour of the counter. When the stepper index matches the index of the
  counter, the carousel stops for a moment under the ejector. At this point
  the M4 signals to the M7 to eject the counter via RPC.
  
  Copywrite 2020 Peter Milne
  License: GPL-3.0 License
*/

#include "RPC_internal.h"

// EasyDriver pins
const int STP = 2;  // STP connected to D2
const int DR = 3;   // DIR connected to D3
const int MS1 = 4;  // MS1 connected to D4
const int MS2 = 5;  // MS2 connected to D5
const int EN = 6;   // EN connected to D6

// Micro-step resolution
const int FULL = 1;
const int HALF = 2;
const int QUATER = 4;
const int EIGTH = 8;

// Directions
const int REVERSE = HIGH; // Reverse rotation
const int FORWARD = LOW;  // Forwards rotation

const int POSITIONS = 5;  // n platter positions
// Number of micro-steps to move one carousel position
const int MICROSTEPS = 200 * EIGTH / POSITIONS;
/*  MS1   MS2   MICRO-STEP RESOLUTION
    LOW   LOW   FULL
    HIGH  LOW   HALF
    LOW   HIGH  QUATER
    HIGH  HIGH  EIGTH
*/
const int DWELL = 200;  // Dwell time in ms

int idx = 1; // Index
int colourIndex = 0;

// Called my M7 to set colour index
int setVar(int a) {
  colourIndex = (int)a;
  return a;
}

// Move stepper by one index position
void moveOne(int *idx, int positions, int steps, int direction) {
  digitalWrite(DR, direction); // Set direction
  delay(1);

  for (int i = 0; i < steps; i++)
  {
    digitalWrite(STP, HIGH); // Trigger one step
    delay(1);
    digitalWrite(STP, LOW); // Pull step pin low to trigger again
    delay(1);
  }
  // Update index
  if (*idx == (positions)) {
    *idx = 1;
    digitalWrite(LEDB, LOW); // Led on
  }
  else {
    (*idx)++;
    digitalWrite(LEDB, HIGH); // Led off
  }
}

void setup() {
  pinMode(LEDB, OUTPUT);
  pinMode(STP, OUTPUT);
  pinMode(DR, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH); // Disable motor
  digitalWrite(MS1, HIGH); // MS1 & MS2 control step resolution
  digitalWrite(MS2, HIGH);
  digitalWrite(EN, LOW); // Enable motor control

  RPC1.begin();
  RPC1.bind("setVar", setVar);
  // Let everything initialise!
  delay(1000);
}

void loop() {
  // Check if stepper index is at correct position to receive counter
  if (idx == colourIndex) {
    // Signal to M7 to eject counter
    RPC1.call("setEjector", 1).as<int>();
    delay(DWELL);
  }
  moveOne(&idx, POSITIONS, MICROSTEPS, FORWARD);
}
