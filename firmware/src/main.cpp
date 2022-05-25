#include <ArduinoJson.h>
#include <FastLED.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiManager.h>

#define LED_STRIP_1_PIN 33
#define LED_STRIP_2_PIN 23
#define LED_STRIP_3_PIN 19
#define LED_STRIP_4_PIN 22
#define LED_STRIP_5_PIN 21
#define LED_STRIP_6_PIN 25

#define NUM_LEDS 144 * 5

#define RESET_BTN_PIN 39
#define RESET_HOLD_DURATION 5 // seconds

#define MQTT_TOPIC_PREFIX "warehouse-locator/"

CRGB led_strip_1[NUM_LEDS];
CRGB led_strip_2[NUM_LEDS];
CRGB led_strip_3[NUM_LEDS];
CRGB led_strip_4[NUM_LEDS];
CRGB led_strip_5[NUM_LEDS];
CRGB led_strip_6[NUM_LEDS];

WiFiManagerParameter custom_mqtt_server("mqttserver", "MQTT server address",
                                        "server", 64);
WiFiManagerParameter custom_mqtt_port("mqttport", "MQTT server port", "1883",
                                      5);
WiFiManagerParameter custom_mqtt_user("mqttuser", "MQTT username", "user", 16);
WiFiManagerParameter custom_mqtt_pass("mqttpass", "MQTT password", "pass", 16);

String device_id = "";

void saveParamsCallback() {
  StaticJsonDocument<200> json;
  json["device_id"] = device_id;
  json["mqtt_server"] = strlen(custom_mqtt_server.getValue()) == 0
                            ? "server"
                            : custom_mqtt_server.getValue();
  json["mqtt_port"] = strlen(custom_mqtt_port.getValue()) == 0
                          ? "port"
                          : custom_mqtt_port.getValue();
  json["mqtt_user"] = strlen(custom_mqtt_user.getValue()) == 0
                          ? "user"
                          : custom_mqtt_user.getValue();
  json["mqtt_pass"] = strlen(custom_mqtt_pass.getValue()) == 0
                          ? "pass"
                          : custom_mqtt_pass.getValue();

  fs::File configFile = SPIFFS.open("/config.json", "w");

  char json_pretty[250];
  serializeJsonPretty(json, json_pretty);
  log_d("%s", json_pretty);

  serializeJson(json, configFile);
  configFile.close();
  ESP.restart();
}

void setup() {
  FastLED.addLeds<WS2812, LED_STRIP_1_PIN, GRB>(led_strip_1, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_2_PIN, GRB>(led_strip_2, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_3_PIN, GRB>(led_strip_3, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_4_PIN, GRB>(led_strip_4, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_5_PIN, GRB>(led_strip_5, NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_6_PIN, GRB>(led_strip_6, NUM_LEDS);

  SPIFFS.begin();
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  if (SPIFFS.exists("/config.json")) {
    fs::File configFile = SPIFFS.open("/config.json", "r");
    DynamicJsonDocument json(configFile.size() * 2);
    auto deserialization_error = deserializeJson(json, configFile);
    if (deserialization_error) {
      SPIFFS.format();
      ESP.restart();
    }
    if (json.containsKey("device_id")) {
      device_id = String((const char *)json["device_id"]);
    }

    if (json.containsKey("mqtt_server")) {
      custom_mqtt_server.setValue(json["mqtt_server"], 64);
    }

    if (json.containsKey("mqtt_port")) {
      custom_mqtt_port.setValue(json["mqtt_port"], 5);
    }

    if (json.containsKey("mqtt_user")) {
      custom_mqtt_user.setValue(json["mqtt_user"], 16);
    }

    if (json.containsKey("mqtt_pass")) {
      custom_mqtt_pass.setValue(json["mqtt_pass"], 16);
    }

    configFile.close();
  } else {
    fs::File configFile = SPIFFS.open("/config.json", "w");

    StaticJsonDocument<200> json;

    srand(time(0));
    device_id = "warehouse-locator-" + rand();
    json["device_id"] = device_id;
    serializeJson(json, configFile);

    configFile.close();
  }

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.setSaveParamsCallback(saveParamsCallback);
  wm.setTitle("Warehouse Locator");

  if (!wm.autoConnect(device_id.c_str(), "lcdporto")) {
    ESP.restart();
  }
}

unsigned long btn_pressed = 0;

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

  if (digitalRead(RESET_BTN_PIN) == HIGH) {
    btn_pressed = millis();
  }
  if (millis() - btn_pressed > RESET_HOLD_DURATION * 1000) {
    SPIFFS.format();
    ESP.restart();
  }
}