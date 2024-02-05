/*
  Portenta H7 Colour Sorter calibration util for M7 Core

  M7 core is responsible for servo control and colour sensor management.

  The servo moves counters from the loader to colour sensor which identifies
  the counter's RGB colour. RGB colours are mapped to a colour index using a
  Euclid's colour distance algorithm:
  
  https://en.wikipedia.org/wiki/Color_difference
  
  M7 updates the M4 colour index via RPC each time it identifies the counter's
  colour.
  
  M7 waits for the eject signal from M4 via RPC, at which point the servo moves
  the counter to the eject position and then returns to the loader position.

  Startup:
  Ejector must be positioned over the ejector hole
  Carousel must have starting cup under ejector hole

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include "RPC.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_PWMServoDriver.h>

const int NUM_SAMPLES = 6;
const int NUM_SENSORS = 4;  // RGBW sensors
uint16_t sample_buffer[NUM_SAMPLES][NUM_SENSORS];

enum colours { WHITE,
               RED,
               YELLOW,
               GREEN,
               VIOLET,
               ORANGE };
const char *colour[] = { "WHITE", "RED", "YELLOW", "GREEN", "VIOLET", "ORANGE" };
volatile boolean eject = false;

const int MAX_COLOUR_DISTANCE = 500;
uint16_t redSensor, greenSensor, blueSensor, clearSensor;  // RGB readings

// Array of average RGB values
const int calibratedColours[][NUM_SENSORS + 1] = {
  { 24, 23, 20, 66, WHITE },
  { 18, 21, 14, 52, GREEN },
  { 15, 14, 12, 41, VIOLET },
  { 21, 17, 14, 51, RED },
  { 27, 18, 15, 59, ORANGE },
  { 30, 27, 18, 74, YELLOW }
};
// Number of samples in the array
const byte calibratedColoursCount = sizeof(calibratedColours) / sizeof(calibratedColours[0]);

// Servo constants
const int SERVO_FREQ = 50;  // For ~50 Hz servos
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

// Print colour sensor RGB values
void printSensor() {
  if (redSensor && greenSensor && blueSensor && clearSensor) {
    Serial.print(redSensor, DEC);
    Serial.print(",");
    Serial.print(greenSensor, DEC);
    Serial.print(",");
    Serial.print(blueSensor, DEC);
    Serial.print(",");
    Serial.println(clearSensor, DEC);
  }
}

// Print colour sensor RGB values
void printBuffer(int i) {
  Serial.print(sample_buffer[i][0], DEC);
  Serial.print(",");
  Serial.print(sample_buffer[i][1], DEC);
  Serial.print(",");
  Serial.print(sample_buffer[i][2], DEC);
  Serial.print(",");
  Serial.println(sample_buffer[i][3], DEC);
}

// Read colour sensor RGB values
void readSensor() {
  tcs.getRawData(&redSensor, &greenSensor, &blueSensor, &clearSensor);
}

// Identify sensor colour sample based on previously calibrated RGB values
// held in the samples array
colours getColourIndex() {
  int colourDistance = MAX_COLOUR_DISTANCE;
  int prevColourDistance = colourDistance;  // Initialise to MAX distance
  colours sample = WHITE;
  // Check the colour distance of the sample against each of the colours in the calibrated control samples
  for (byte i = 0; i < calibratedColoursCount; i++) {
    Serial.print("Sample: ");
    Serial.print(i);
    Serial.print(" ");
    colourDistance = getColourDistance(redSensor, greenSensor, blueSensor, calibratedColours[i][0], calibratedColours[i][1], calibratedColours[i][2]);
    // If this sample has a lower colour distance than the previous sample from the control array, set it to the next
    // colour from the control array ( ie it is a better match )
    if (colourDistance < prevColourDistance) {
      sample = (colours)calibratedColours[i][4];
      prevColourDistance = colourDistance;
    }
    Serial.print(colourDistance);
    Serial.print(",");
    Serial.println(colour[sample]);
  }
  return sample;
}

// Calculate Euclid's colour distance between two RGB colours
float getColourDistance(int redSensor, int greenSensor, int blueSensor, int redSample, int greenSample, int blueSample) {
  return sqrt(pow(redSensor - redSample, 2) + pow(greenSensor - greenSample, 2) + pow(blueSensor - blueSample, 2));
}

// Move servo to given position
void moveTo(int pos) {
  pwm.setPWM(SERVO_NUM, 0, pos);
}

void setup() {
  RPC.begin();
  RPC.bind("setEjector", setEjector);
  bootM4();
  tcs.begin();
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(5000);
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println(calibratedColoursCount);
  int i = 0;
  //for (int i = 0; i < NUM_SAMPLES; i++) {
  while (i < NUM_SAMPLES) {
    moveTo(LOADER_POSITION);
    delay(600);
    moveTo(SENSOR_POSITION);
    delay(1000);
    readSensor();
    delay(5);
    if (redSensor && greenSensor && blueSensor && clearSensor) {
      sample_buffer[i][0] = redSensor;
      sample_buffer[i][1] = greenSensor;
      sample_buffer[i][2] = blueSensor;
      sample_buffer[i][3] = clearSensor;
      printSensor();
      printBuffer(i);
      i++;
      int colourIndex = (int)getColourIndex();
      Serial.println("Colour: " + String(colour[colourIndex]));
    }
    eject = false;
    moveTo(EJECTOR_POSITION);
    delay(600);  // Allow servo to reach ejector
  }
  int idx = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    for (int j = 0; j < NUM_SAMPLES; j++) {
      idx += sample_buffer[j][i];
    }
    Serial.print(idx / NUM_SAMPLES);
    Serial.print(",");
    idx = 0;
  }
}

void loop() {}
