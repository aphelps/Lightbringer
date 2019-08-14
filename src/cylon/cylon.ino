#include "FastLED.h"

#ifndef NUM_LEDS
// How many leds in your strip?
  #define NUM_LEDS 69*5
#endif

#ifndef BRIGHTNESS
  #define BRIGHTNESS 128
#endif

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() {
  boolean initialized = false;

  Serial.begin(9600);
  Serial.println("Lightbringer cylon");
#ifdef ATMEGA328_SOFT
  initialized = true;
  LEDS.addLeds<APA102, 11, 13, RGB, DATA_RATE_MHZ(1)>(leds, NUM_LEDS);
#endif
#ifdef ATMEGA1284_SOFT
  initialized = true;
  LEDS.addLeds<APA102, 5, 7, RGB, DATA_RATE_MHZ(1)>(leds, NUM_LEDS);
#endif
#ifdef PIXELS_APA102_12_8
  initialized = true;
  LEDS.addLeds<APA102, 12, 8, RGB, DATA_RATE_MHZ(1)>(leds, NUM_LEDS);
#endif

  if (!initialized) {
    LEDS.addLeds<APA102, 11, 13, RGB, DATA_RATE_MHZ(1)>(leds, NUM_LEDS);
  }

  LEDS.setBrightness(BRIGHTNESS);

  Serial.print("NUMLEDS:");
  Serial.println(NUM_LEDS);
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void loop() {
  static uint8_t hue = 0;
  Serial.print("x");
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  Serial.print("x");

  // Now go in the other direction.
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}