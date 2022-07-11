#include <ArduinoJson.h>
#include <FastLED.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiManager.h>

#define LED_STRIP_1_PIN 19
#define LED_STRIP_2_PIN 22
#define LED_STRIP_3_PIN 19
#define LED_STRIP_4_PIN 22
#define LED_STRIP_5_PIN 21
#define LED_STRIP_6_PIN 25

#define NUM_LEDS 144 * 5
#define NUM_STRIPS 6

#define CONFIG_FILE_PATH "/config.json"
#define ID_FILE_PATH "/device-id.txt"

#define MQTT_TOPIC_PREFIX "warehouse-locator/"

char device_id[11] = "";

CRGB led_strips[NUM_STRIPS][NUM_LEDS];

WiFiManagerParameter custom_mqtt_server("mqttserver", "MQTT server address",
                                        "server", 64);
WiFiManagerParameter custom_mqtt_port("mqttport", "MQTT server port", "1883",
                                      5);
WiFiManagerParameter custom_mqtt_user("mqttuser", "MQTT username", "user", 16);
WiFiManagerParameter custom_mqtt_pass("mqttpass", "MQTT password", "pass", 16);

WiFiManager wm;
WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);

void mqtt_receive(char *topic, byte *payload, unsigned int length) {
  log_d("Message arrived [%s]: %s", topic, strndup((char *)payload, length));

  char *topic_prefix = strtok(topic, "/");
  char *dev_id = strtok(NULL, "/");
  uint8_t strip_n = atoi(strtok(NULL, "/"));
  uint8_t led_n = atoi(strtok(NULL, "/"));

  if (strcmp(dev_id, device_id) != 0)
    return;

  led_strips[strip_n][led_n] = strncmp((char *)payload, "on", length) == 0
                                   ? CRGB(255, 255, 255)
                                   : CRGB(0, 0, 0);
  FastLED.show();
}

bool mqtt_connect() {
  if (mqtt.connected())
    return true;

  bool status = mqtt.connect(device_id, custom_mqtt_user.getValue(),
                             custom_mqtt_pass.getValue());

  if (status == false) {
    return false;
  }
  char topic[64];
  sprintf(topic, "%s%s/+/+", MQTT_TOPIC_PREFIX, device_id);
  mqtt.subscribe(topic);

  log_i("Connected to MQTT server: %s:%s", custom_mqtt_server.getValue(),
        custom_mqtt_port.getValue());
  return mqtt.connected();
}

void saveConfigCallback() {
  StaticJsonDocument<128> json;
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

  fs::File configFile = SPIFFS.open(CONFIG_FILE_PATH, "w");

  char json_pretty[150];
  serializeJsonPretty(json, json_pretty);
  log_d("%s", json_pretty);

  serializeJson(json, configFile);
  configFile.close();
  ESP.restart();
}

void setup() {
  FastLED.addLeds<WS2812, LED_STRIP_1_PIN, GRB>(led_strips[0], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_2_PIN, GRB>(led_strips[1], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_3_PIN, GRB>(led_strips[2], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_4_PIN, GRB>(led_strips[3], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_5_PIN, GRB>(led_strips[4], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_6_PIN, GRB>(led_strips[5], NUM_LEDS);

  WiFi.mode(WIFI_STA);
  SPIFFS.begin(true);

  if (SPIFFS.exists(ID_FILE_PATH)) {
    fs::File device_id_file = SPIFFS.open(ID_FILE_PATH, "r");
    device_id_file.readBytes(device_id, 11);
    device_id_file.close();
  } else {
    fs::File device_id_file = SPIFFS.open(ID_FILE_PATH, "w");
    sprintf(device_id, "%u", esp_random());
    device_id_file.print(device_id);
    device_id_file.close();
  }
  log_d("Device ID: %s", device_id);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setTitle("Warehouse Locator");

  char portal_ssid[32] = "";
  sprintf(portal_ssid, "warehouse-locator-%s", device_id);
  wm.autoConnect(portal_ssid);

  if (SPIFFS.exists(CONFIG_FILE_PATH)) {
    fs::File configFile = SPIFFS.open(CONFIG_FILE_PATH, "r");
    DynamicJsonDocument json(configFile.size() * 2);
    auto deserialization_error = deserializeJson(json, configFile);
    if (deserialization_error) {
      SPIFFS.format();
      ESP.restart();
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
    log_d("Config JSON not found");
    wm.startConfigPortal(portal_ssid, "lcdporto");
  }

  mqtt.setServer(custom_mqtt_server.getValue(),
                 atoi(custom_mqtt_port.getValue()));

  mqtt.setCallback(mqtt_receive);
  while (mqtt_connect() == false) {
    delay(500);
    log_d("Failed to connect to MQTT. Retrying...");
  }

  char topic[64];
  sprintf(topic, "%s%s", MQTT_TOPIC_PREFIX, device_id);
  mqtt.publish(topic, "online");
}

void loop() {
  if (mqtt_connect())
    mqtt.loop();
}