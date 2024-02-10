/* 
  Colour utilities for use with ColourSorter

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include "ColourUtils.h"

// Calculate Euclid's colour distance between two RGB colours
float getColourDistance(int rSensor, int gSensor, int bSensor, int rSample, int gSample, int bSample) {
  return sqrt(pow(rSensor - rSample, 2) + pow(gSensor - gSample, 2) + pow(bSensor - bSample, 2));
}

// Identify colour sample based on previously calibrated RGB values in the calibratedColours array
colours getColourIndex(int rSensor, int gSensor, int bSensor) {
  int colourDistance = MAX_COLOUR_DISTANCE;
  int prevColourDistance = colourDistance;  // Initialise to MAX distance
  colours sample = WHITE;
  // Check colour distance of sample against each colours in the calibated control samples
  for (byte i = 0; i < NUM_COLOURS; i++) {
    // Serial.print("Sample: ");
    // Serial.print(i);
    // Serial.print(" ");
    colourDistance = getColourDistance(rSensor, gSensor, bSensor, calibratedColours[i][0], calibratedColours[i][1], calibratedColours[i][2]);
    // If this sample has a lower colour distance than the previous sample set previous sample to crrent sample ( ie it is a better match )
    if (colourDistance < prevColourDistance) {
      sample = (colours)calibratedColours[i][4];
      prevColourDistance = colourDistance;
    }
    // Serial.print(colourDistance);
    // Serial.print(",");
    // Serial.println(colour[sample]);
  }
  return sample;
}

// Print out sensor RGBC values
void printSensor(uint16_t rSensor, uint16_t gSensor, uint16_t bSensor, uint16_t cSensor) {
  Serial.print((int)rSensor, DEC);
  Serial.print(",");
  Serial.print((int)gSensor, DEC);
  Serial.print(",");
  Serial.print((int)bSensor, DEC);
  Serial.print(",");
  Serial.print((int)cSensor, DEC);
}
