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

  Startup:
  Ejector must be positioned over the ejector hole
  Carousel must have starting cup under ejector hole

  Copywrite 2023 Peter Milne
  License: GPL-3.0 License
*/

#include "RPC.h"
#include "Adafruit_TCS34725.h"
#include <Adafruit_PWMServoDriver.h>
#include "ColourUtils.h"

volatile boolean eject = false;
uint16_t redSensor, greenSensor, blueSensor, clearSensor;  // RGBC readings

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

// Read colour sensor RGB values
void readSensor() {
  tcs.getRawData(&redSensor, &greenSensor, &blueSensor, &clearSensor);
  Serial.print(redSensor, DEC);
  Serial.print(",");
  Serial.print(greenSensor, DEC);
  Serial.print(",");
  Serial.print(blueSensor, DEC);
  Serial.print(",");
  Serial.println(clearSensor, DEC);
}

// Move servo to given position
void moveTo(int pos) {
  pwm.setPWM(SERVO_NUM, 0, pos);
}

void setup() {
  RPC.begin();
  RPC.bind("setEjector", setEjector);
  bootM4();
  Serial.begin(115200);
  tcs.begin();
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(5000);
  // Warm up sensor
  readSensor();
  delay(1000);
  Serial.println("Starting...");
}

void loop() {
  moveTo(LOADER_POSITION);
  delay(600);

  moveTo(SENSOR_POSITION);
  delay(1000);
  readSensor();
  int colourIndex = (int)getColourIndex(redSensor, greenSensor, blueSensor);
  Serial.println("Colour: " + String(colour[colourIndex]));
  if (colourIndex > 0) {
    // Update M4 colour index
    RPC.call("setColourIndex", colourIndex).as<int>();
    while (!eject)  // Wait for ejection signal from M4
      ;
    eject = false;
    moveTo(EJECTOR_POSITION);
    Serial.println("Ejected...");
    delay(600);  // Allow servo to reach ejector
  }
}
