/*
  Colour Sorter calibration util

  Startup:
  Fill hopper with several samples of the same colour.
  Reset the MCU and apply power for servo.
  Open the serial monitor to start the process.

  The loader will keep loading samples until it sees a "White" sample.

  The first output are the RGBC sensor readings and the colour according to the calibration
  table in ColourUtils.h

  The second reading is the average RGBC of all the samples - use this to update the calibraion array.
  Repeat for each colour then reflash.

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Adafruit_PWMServoDriver.h>
#include "ColourUtils.h"

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X);

volatile boolean eject = false;
uint16_t redSensor, greenSensor, blueSensor, clearSensor;  // RGB readings
#define MAX_SAMPLES 20
uint16_t sample_buffer[MAX_SAMPLES][NUM_SENSORS];

// Servo constants
const int SERVO_FREQ = 50;  // For ~50 Hz servos
const int SERVO_NUM = 0;

// Adjust position values accordingly
int LOADER_POSITION = 168;
int SENSOR_POSITION = 228;
int EJECTOR_POSITION = 292;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

bool start = true;
colours sample = WHITE;
int num_samples = 0;

// Move servo to given position
void moveTo(int pos) {
  pwm.setPWM(SERVO_NUM, 0, pos);
}

void printColour() {
  printSensor(redSensor, greenSensor, blueSensor, clearSensor);
  Serial.print(",");
  sample = getColourIndex(redSensor, greenSensor, blueSensor);
  Serial.println(colour[sample]);
}

void setup(void) {
  tcs.begin();
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  Serial.begin(115200);
  while (!Serial)
    ;
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("TCS34725 not found ...");
    while (1)
      ;
  }
}

void loop(void) {
  while (start) {
    moveTo(LOADER_POSITION);
    delay(600);
    moveTo(SENSOR_POSITION);
    delay(1000);

    tcs.getRawData(&redSensor, &greenSensor, &blueSensor, &clearSensor);
    printColour();

    int idx = 0;
    // Keep sampling until a white sample
    if ((sample != WHITE) && (num_samples < MAX_SAMPLES)) {
      sample_buffer[num_samples][0] = redSensor;
      sample_buffer[num_samples][1] = greenSensor;
      sample_buffer[num_samples][2] = blueSensor;
      sample_buffer[num_samples][3] = clearSensor;
      num_samples++;
      // Average samples
      for (int i = 0; i < NUM_SENSORS; i++) {
        for (int j = 0; j <= num_samples; j++) {
          idx += sample_buffer[j][i];
        }
        Serial.print(idx / num_samples);
        Serial.print(",");
        Serial.print("");
        idx = 0;
      }
      Serial.println();
      moveTo(EJECTOR_POSITION);
      delay(600);  // Allow servo to reach ejector

    } else start = false;
  }
}
