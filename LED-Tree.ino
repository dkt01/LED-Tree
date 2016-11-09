#include <Adafruit_NeoPixel.h>
#include "NeoPixelRing.h"
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 9

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
const uint8_t ringsArray[6] = { 32, 24, 16, 12, 8, 1 };
NeoPixelRing tree = NeoPixelRing(PIN, NEO_GRB + NEO_KHZ800, 6, ringsArray);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup()
{
  Serial.begin(9600);
  tree.begin();
  tree.setBrightness(75);
  tree.update(); // Initialize all pixels to 'off'
  tree.setPattern(0,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(1,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(2,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(3,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(4,
                  NeoPixelRing::SPIN,
                  0x00FF00,
                  2000,
                  0);
  tree.setPattern(5,
                  NeoPixelRing::RAINBOW,
                  0x00FF00,
                  4000,
                  0);
}

void loop()
{
  // static uint8_t progress = 0;
  // static uint8_t loopCount = 0;
  // if(loopCount % 10 == 0)
  // {
  //   progress++;
  //   progress = progress % 100;
  //   tree.setPattern(2,
  //                   NeoPixelRing::PROGRESS,
  //                   0x0000FF,
  //                   2000,
  //                   progress);
  // }
  // loopCount++;
  tree.update();
  delay(10);
}
