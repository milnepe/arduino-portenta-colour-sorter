/* 
  Colour utilities for use with ColourSorter

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include <Arduino.h>

#define NUM_COLOURS 6  // Number of colours recognised including white
#define NUM_SENSORS 4  // Number of RGBC sensors
#define MAX_COLOUR_DISTANCE 500
typedef enum { WHITE,
               RED,
               YELLOW,
               GREEN,
               VIOLET,
               ORANGE } colours;

// Array of average RGBC values
const int calibratedColours[NUM_COLOURS][NUM_SENSORS + 1] = {
  { 27, 26, 23, 72, WHITE },
  { 16, 20, 12, 47, GREEN },
  { 12, 9, 9, 30, VIOLET },
  { 22, 11, 10, 41, RED },
  { 31, 16, 13, 59, ORANGE },
  { 35, 28, 18, 79, YELLOW }
};

static const char *colour[NUM_COLOURS] = { "WHITE", "RED", "YELLOW", "GREEN", "VIOLET", "ORANGE" };

void printSensor(uint16_t rSensor, uint16_t gSensor, uint16_t bSensor, uint16_t cSensor);
colours getColourIndex(int rSensor, int gSensor, int bSensor);
