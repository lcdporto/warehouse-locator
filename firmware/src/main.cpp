#include <ArduinoJson.h>
#include <FastLED.h>
#define MQTT_MAX_PACKET_SIZE 2048
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <mutex>

#define LED_STRIP_1_PIN 15
#define LED_STRIP_2_PIN 16
#define LED_STRIP_3_PIN 19
#define LED_STRIP_4_PIN 22
#define LED_STRIP_5_PIN 21
#define LED_STRIP_6_PIN 25

#define NUM_LEDS 144 * 5
#define NUM_STRIPS 6

#define CONFIG_FILE_PATH "/config.json"
#define ID_FILE_PATH "/device-id.txt"

#define MQTT_TOPIC_PREFIX "warehouse-locator/"

#define MQTT_ANNOUNCE_PERIOD 30 // in seconds

#define LED_OFF_PAYLOAD "{\"color\":{\"r\": 0,\"g\": 0,\"b\":0}}"

char device_id[11] = "";

static CRGB led_strips[NUM_STRIPS][NUM_LEDS];
SemaphoreHandle_t led_mtx = xSemaphoreCreateMutex();

WiFiManagerParameter custom_mqtt_server("mqttserver", "MQTT server address",
                                        "server", 64);
WiFiManagerParameter custom_mqtt_port("mqttport", "MQTT server port", "1883",
                                      5);
WiFiManagerParameter custom_mqtt_user("mqttuser", "MQTT username", "user", 16);
WiFiManagerParameter custom_mqtt_pass("mqttpass", "MQTT password", "pass", 64);

WiFiManager wm;
WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);

typedef struct {
  uint8_t strip;
  uint16_t led;
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;
  uint16_t timeout = 5;
} led_ctrl;

void control_LED(void *led_struct) {
  led_ctrl led;
  memcpy(&led, led_struct, sizeof(led_ctrl));

  char topic[64];
  sprintf(topic, "%s%s/%d/%d/state", MQTT_TOPIC_PREFIX, device_id, led.strip,
          led.led);
  log_d("Publishing to state topic: %s", topic);

  log_d("Turned strip %u led %u on", led.strip, led.led);
  xSemaphoreTake(led_mtx, portMAX_DELAY);
  delay(1);
  led_strips[led.strip][led.led] = CRGB(led.r, led.g, led.b);
  delay(1);
  FastLED.show();
  delay(1);
  xSemaphoreGive(led_mtx);

  char payload[64];
  sprintf(payload, "{\"color\":{\"r\":%u,\"g\":%u,\"b\":%u}}", led.r, led.g,
          led.b);
  mqtt.publish(topic, payload, true);

  vTaskDelay(led.timeout * 1000 / portTICK_PERIOD_MS);

  xSemaphoreTake(led_mtx, portMAX_DELAY);
  delay(1);
  led_strips[led.strip][led.led] = CRGB(0, 0, 0);
  delay(1);
  FastLED.show();
  delay(1);
  xSemaphoreGive(led_mtx);

  mqtt.publish(topic, LED_OFF_PAYLOAD, true);
  log_d("Turned strip %u led %u off", led.strip, led.led);

  vTaskDelete(NULL);
}

void mqtt_receive(char *topic, byte *payload, unsigned int length) {
  log_d("Message arrived [%s]: %s", topic, strndup((char *)payload, length));

  // skip prefix, device ID and forward slash
  topic += strlen(MQTT_TOPIC_PREFIX) + 10 + 1;

  log_d("Trimmed topic to %s", topic);

  char *token = strtok(topic, "/");
  if (strcmp(token, "multiple") != 0) {
    uint8_t strip_n;
    uint16_t led_n;

    strip_n = atoi(token);
    led_n = atoi(strtok(NULL, "/"));

    log_d("Addressed strip %u and led %u", strip_n, led_n);

    DynamicJsonDocument doc(length * 2);
    if (deserializeJson(doc, (char *)payload)) {
      log_e("Received invalid JSON");
      return;
    }
    log_d("Deserialized JSON successfully");

    uint16_t timeout;
    if (doc.containsKey("timeout")) {
      timeout = doc["timeout"];
    } else {
      timeout = 5;
    }

    led_ctrl led;
    led.strip = strip_n;
    led.led = led_n;
    led.r = doc["color"]["r"];
    led.g = doc["color"]["g"];
    led.b = doc["color"]["b"];
    led.timeout = timeout;

    log_d(
        "Will turn on the strip %u led %u with color (%u,%u,%u) for %u seconds",
        led.strip, led.led, led.r, led.g, led.b, led.timeout);

    xTaskCreate(control_LED, "control_LED", 2048, (void *)&led, 1, NULL);
    delay(1);
  } else {
    DynamicJsonDocument doc(length * 2);
    if (deserializeJson(doc, (char *)payload)) {
      log_e("Received invalid JSON");
      return;
    }
    log_d("Deserialized JSON successfully");
    log_d("Doc has size: %d", doc.size());

    for (uint8_t i = 0; i < doc.size(); i++) {

      uint16_t timeout;
      if (doc[i].containsKey("timeout")) {
        timeout = doc[i]["timeout"];
      } else {
        timeout = 5;
      }

      led_ctrl led;
      led.strip = doc[i]["strip"];
      led.led = doc[i]["led"];
      led.r = doc[i]["color"]["r"];
      led.g = doc[i]["color"]["g"];
      led.b = doc[i]["color"]["b"];
      led.timeout = timeout;

      xTaskCreate(control_LED, "control_LED", 2048, (void *)&led, 1, NULL);
      delay(1);
    }
  }
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
  log_v("Subscribed to topic: %s", topic);

  sprintf(topic, "%s%s/multiple", MQTT_TOPIC_PREFIX, device_id);
  mqtt.subscribe(topic);
  log_v("Subscribed to topic: %s", topic);

  log_i("Connected to MQTT server: %s:%s", custom_mqtt_server.getValue(),
        custom_mqtt_port.getValue());
  return mqtt.connected();
}

void mqtt_announce(void *) {
  while (1) {
    if (mqtt_connect()) {
      char topic[64];
      sprintf(topic, "%s%s", MQTT_TOPIC_PREFIX, device_id);
      mqtt.publish(topic, "online");
    }
    vTaskDelay(MQTT_ANNOUNCE_PERIOD * 1000 / portTICK_PERIOD_MS);
  }
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
    sprintf(device_id, "%010u", esp_random());
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

  bool configs_present = false;
  if (SPIFFS.exists(CONFIG_FILE_PATH)) {
    configs_present = true;
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
      custom_mqtt_pass.setValue(json["mqtt_pass"], 64);
    }

    configFile.close();
  } else {
    log_d("Config JSON not found");
    configs_present = false;
  }

  char portal_ssid[32] = "";
  sprintf(portal_ssid, "warehouse-locator-%s", device_id);
  wm.autoConnect(portal_ssid);

  if (configs_present == false) {
    wm.startConfigPortal(portal_ssid);
  }

  mqtt.setServer(custom_mqtt_server.getValue(),
                 atoi(custom_mqtt_port.getValue()));

  mqtt.setCallback(mqtt_receive);

  if (mqtt_connect() == false) {
    log_d("Failed to connect to MQTT. Starting config portal");
    wm.setConfigPortalTimeout(5 * 60);
    wm.startConfigPortal(portal_ssid);
    ESP.restart();
  }

  xTaskCreate(mqtt_announce, "announce_ID", 2048, NULL, 1, NULL);
}

void loop() {
  if (mqtt_connect()) {
    mqtt.loop();
  } else {
    delay(1000);
  }
}
