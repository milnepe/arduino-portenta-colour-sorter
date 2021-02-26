/*
  Portenta H7 Colour Sorter for M7 Core

  M7 core is responsible for servo control and colour sensor management.

  The servo moves counters from the loader to colour sensor which identifies
  the counter's RGB colour. RGB colours are mapped to a colour index using a
  Euclid's colour distance algorithm:
  
  https://en.wikipedia.org/wiki/Color_difference
  
  M7 updates the M4 colour index via RPC each time it identifies the counter's
  colour.
  
  M7 waits for the eject signal from M4 via RPC, at which point the servo moves
  the counter to the eject position and then returns to the loader position.

  Copywrite 2020 Peter Milne
  License: GPL-3.0 License
*/

#include "RPC_internal.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_PWMServoDriver.h>

enum colours {WHITE, RED, YELLOW, GREEN, VIOLET, ORANGE};
char *colour[] = {"WHITE", "RED", "YELLOW",  "GREEN", "VIOLET", "ORANGE"};
int colourIndex = 0;
boolean eject = false;

const int MAX_COLOUR_DISTANCE = 500;
uint16_t redSensor, greenSensor, blueSensor, clearSensor; // RGB readings

// Array of average RGB values
const int SAMPLES[][4] = {
  {23, 22, 19, WHITE},
  {16, 19, 12, GREEN},
  {11, 8, 8, VIOLET},
  {19, 10, 9, RED},
  {24, 16, 12, ORANGE},
  {27, 22, 16, YELLOW}
};
// Number of samples in the array
const byte samplesCount = sizeof(SAMPLES) / sizeof(SAMPLES[0]);

// Servo constants
const int SERVO_FREQ = 50; // For ~50 Hz servos
const int SERVO_NUM = 0;

// Adjust position values accordingly
int LOADER_POSITION = 168;
int SENSOR_POSITION = 228;
int EJECTOR_POSITION = 292;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);

// Called by M4 to signal ejection
int setEjector(int a) {
  eject = (boolean)a;
  return a;
}

// Read colour sensor RGB values
void readSensor()
{
  tcs.getRawData(&redSensor, &greenSensor, &blueSensor, &clearSensor);
  Serial.print(redSensor, DEC); Serial.print(",");
  Serial.print(greenSensor, DEC); Serial.print(",");
  Serial.print(blueSensor, DEC); Serial.print(",");
  Serial.println(clearSensor, DEC);
}

// Identify the actual colour of sample based on previously calibrated RGB values
// held in the samples array
colours identifySample()
{
  int colourDistance = MAX_COLOUR_DISTANCE;
  int prevColourDistance = colourDistance;  // Initialise to MAX distance
  colours sample = WHITE;
  // Check the colour distance of the sample against each of the colours in the calibated control samples
  for (byte i = 0; i < samplesCount; i++)
  {
    Serial.print("Sample: "); Serial.print(i); Serial.print(" ");
    colourDistance = getColourDistance(redSensor, greenSensor, blueSensor, SAMPLES[i][0], SAMPLES[i][1], SAMPLES[i][2]);
    // If this sample has a lower colour distance than the previous sample from the control array, set it to the next
    // colour from the control array ( ie it is a better match ) 
    if (colourDistance < prevColourDistance)
    {
      sample = (colours)SAMPLES[i][3];
      prevColourDistance = colourDistance;
    }
    Serial.print(colourDistance); Serial.print(","); Serial.println(colour[sample]);
  }
  return sample;
}

// Calculate Euclid's colour distance between two RGB colours 
float getColourDistance(int redSensor, int greenSensor, int blueSensor, int redSample, int greenSample, int blueSample)
{
  return sqrt(pow(redSensor - redSample, 2) + pow(greenSensor - greenSample, 2) + pow(blueSensor - blueSample, 2));
}

// Move servo to given position
void moveTo(int pos) {
  pwm.setPWM(SERVO_NUM, 0, pos);
}

void setup() {
  RPC1.begin();
  RPC1.bind("setEjector", setEjector);
  bootM4();
  Serial.begin(115200);
  tcs.begin();
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(5000);
}

void loop() {
  Serial.println("**********");

  moveTo(LOADER_POSITION);
  delay(600);

  moveTo(SENSOR_POSITION);
  delay(600);
  readSensor();
  colours sample = identifySample();
  if (sample == WHITE) {
    eject = true;
  }
  else {
    colourIndex = (int)sample;
  }
  Serial.println("Colour: " + String(colour[colourIndex]));
  // Update M4 colour index
  RPC1.call("setVar", colourIndex).as<int>();

  // Wait for ejection signal from M4
  while (!eject) delay(5);

  moveTo(EJECTOR_POSITION);
  eject = false;
  colourIndex = 0;
  delay(600);
  // Reset M4 colour index
  RPC1.call("setVar", colourIndex).as<int>();
}
