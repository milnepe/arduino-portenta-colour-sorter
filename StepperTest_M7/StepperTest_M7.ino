/*
  Portenta H7 Stepper motor test for M7 Core

  M7 core is responsible for passing the index for the stepper motor
  controlled by the M4 core.

  Reads a colour from Serial input (until Newline!) and returns the color index to pass to M4

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include "Arduino.h"
#include "RPC.h"

using namespace rtos;

const char *colour[] = { "WHITE", "RED", "YELLOW", "GREEN", "VIOLET", "ORANGE" };
const int NUM_COLOURS = 6;
const int MAX_MESSAGE_LENGTH = 8;
char msgbuffer[MAX_MESSAGE_LENGTH];
volatile boolean eject = false;

// Called by M4 to signal ejection
int setEjector(int a) {
  eject = (boolean)a;
  return a;
}

// Return index of given colour
int getColourIdx(String colour_str) {
  for (int idx = 0; idx < NUM_COLOURS; idx++) {
    String index_colour(colour[idx]);
    if (index_colour.equals(colour_str)) {
      return idx;
    }
  }
  return -1;
}

void setup() {
  RPC.begin();
  RPC.bind("setEjector", setEjector);
  bootM4();
  pinMode(PD_5, OUTPUT);  // HAT LED
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Stepper test");
}

void loop() {
  digitalWrite(PD_5, LOW);
  while (Serial.available() > 0) {
    static unsigned int msg_pos = 0;
    char inByte = Serial.read();
    if (inByte != '\n' && (msg_pos < MAX_MESSAGE_LENGTH - 1)) {
      msgbuffer[msg_pos] = inByte;
      msg_pos++;
    }
    // Message received
    else {
      msgbuffer[msg_pos] = '\0';
      msg_pos = 0;  // Reset buffer

      int colourIndex = getColourIdx(String(msgbuffer));
      digitalWrite(PD_5, HIGH);
      Serial.print(colourIndex);
      Serial.print(" ");
      Serial.println(msgbuffer);
      if (colourIndex > 0) {
        // Update M4 colour index
        RPC.call("setColourIndex", colourIndex).as<int>();
        // Wait for ejection signal from M4
        while (!eject)
          ;
        eject = false;
        Serial.println("Ejected...");
      }
    }
  }
}
