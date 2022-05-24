#include <FastLED.h>

#define LED_PIN 33
#define NUM_LEDS 300

CRGB leds[NUM_LEDS];

void setup()
{
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
}

void loop()
{
  for (uint8_t i = 0; i < 300; i += 19)
  {
    for (uint8_t j = 0; j < 19; j++)
    {
      leds[i + j] = CRGB(255, 255, 255);
    }
    FastLED.show();
    delay(1000);
    for (uint8_t j = 0; j < 19; j++)
    {
      leds[i + j] = CRGB(0, 0, 0);
    }
  }
}