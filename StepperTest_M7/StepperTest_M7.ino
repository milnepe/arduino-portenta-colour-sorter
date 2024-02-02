/*
  Portenta H7 Stepper motor test for M7 Core

  M7 core is responsible for passing the index for the stepper motor
  controlled by the M4 core.

  Reads a colour from Serial input (until Newline!) and returns the color index to pass to M4

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include "RPC.h"

enum colours { INVALID,
               WHITE,
               RED,
               YELLOW,
               GREEN,
               VIOLET,
               ORANGE };

const char *colour[] = { "INVALID", "WHITE", "RED", "YELLOW", "GREEN", "VIOLET", "ORANGE" };
const int MAX_MESSAGE_LENGTH = 8;
char msgbuffer[MAX_MESSAGE_LENGTH];
volatile boolean eject = false;
// const int hatLed = PD_5;

// Called by M4 to signal ejection
int setEjector(int a) {
  eject = (boolean)a;
  return a;
}

// Return index of given colour
int getColourIdx(String colour_str) {
  for (int idx = 0; idx < 7; idx++) {
    String index_colour(colour[idx]);
    if (index_colour.equals(colour_str)) {
      return idx;
    }
  }
  return 0;
}

void setup() {
  // pinMode(hatLed, OUTPUT);
  RPC.begin();
  RPC.bind("setEjector", setEjector);
  bootM4();
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Stepper test");
}

void loop() {
  // digitalWrite(hatLed, LOW);
  while (Serial.available() > 0) {
    static unsigned int msg_pos = 0;
    char inByte = Serial.read();
    if (inByte != '\n' && (msg_pos < MAX_MESSAGE_LENGTH - 1)) {
      msgbuffer[msg_pos] = inByte;
      msg_pos++;
    }
    // Message received
    else {
      // digitalWrite(hatLed, HIGH);
      msgbuffer[msg_pos] = '\0';
      msg_pos = 0;  // Reset buffer
      int colourIndex = getColourIdx(String(msgbuffer));
      // Update M4 colour index
      RPC.call("setVar", colourIndex).as<int>();
      Serial.print(colourIndex);
      Serial.print(" ");
      Serial.println(msgbuffer);

      // Wait for ejection signal from M4
      while (!eject)
        ;  // Wait for ejector signal
      eject = false;
      colourIndex = 0;
      delay(600);
      // Reset M4 colour index
      RPC.call("setVar", colourIndex).as<int>();
    }
  }
  /*
  readSensor();
  colours sample = identifySample();
  if (sample == WHITE) {
    eject = true;
  } else {
    colourIndex = (int)sample;
  }
  Serial.println("Colour: " + String(colour[colourIndex]));
  // Update M4 colour index
  RPC.call("setVar", colourIndex).as<int>();

  // Wait for ejection signal from M4
  while (!eject) delay(5);

  eject = false;
  colourIndex = 0;
  delay(600);
  // Reset M4 colour index
  RPC.call("setVar", colourIndex).as<int>();
*/
}
