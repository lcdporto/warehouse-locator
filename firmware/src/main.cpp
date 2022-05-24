#include <FastLED.h>

#define LED_STRIP_1_PIN 33
#define LED_STRIP_2_PIN 23
#define LED_STRIP_3_PIN 19
#define LED_STRIP_4_PIN 22
#define LED_STRIP_5_PIN 21
#define LED_STRIP_6_PIN 25

#define NUM_LEDS 144 * 5

CRGB led_strip_1[NUM_LEDS];
CRGB led_strip_2[NUM_LEDS];
CRGB led_strip_3[NUM_LEDS];
CRGB led_strip_4[NUM_LEDS];
CRGB led_strip_5[NUM_LEDS];
CRGB led_strip_6[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2812, LED_STRIP_1_PIN, GRB>(led_strip_1, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_2_PIN, GRB>(led_strip_2, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_3_PIN, GRB>(led_strip_3, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_4_PIN, GRB>(led_strip_4, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_5_PIN, GRB>(led_strip_5, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_6_PIN, GRB>(led_strip_6, NUM_LEDS);
}

void loop() {
  for (uint8_t i = 0; i < 300; i += 19) {
    for (uint8_t j = 0; j < 19; j++) {
      led_strip_1[i + j] = CRGB(255, 255, 255);
    }
    FastLED.show();
    delay(1000);
    for (uint8_t j = 0; j < 19; j++) {
      led_strip_1[i + j] = CRGB(0, 0, 0);
    }
  }
}