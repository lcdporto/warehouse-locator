#include "WiFi.h"
#include <ArduinoJson.h>
#include <EthernetENC.h>
#include <FastLED.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <mutex>

#define LED_STRIP_1_PIN 2
#define LED_STRIP_2_PIN 4
#define LED_STRIP_3_PIN 16
#define LED_STRIP_4_PIN 17
#define LED_STRIP_5_PIN 21
#define LED_STRIP_6_PIN 22

#define NUM_LEDS 144 * 5
#define NUM_STRIPS 6

#define CONFIG_FILE_PATH "/config.json"
#define ID_FILE_PATH "/device-id.txt"

#define MQTT_TOPIC_PREFIX "warehouse-locator/"

#define MQTT_ANNOUNCE_PERIOD 30 // in seconds

#define LED_OFF_PAYLOAD "{\"color\":{\"r\": 0,\"g\": 0,\"b\":0}}"

#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_USER "mqtt_user"
#define MQTT_PASS "mqtt_password"

#define WiFi_SSID "SSID"
#define WiFi_PASS "password"

#define LINK_REFRESH_INTERVAL 1000 // milliseconds

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

char device_id[11] = "";

static CRGB led_strips[NUM_STRIPS][NUM_LEDS];
SemaphoreHandle_t led_mtx = xSemaphoreCreateMutex();

WiFiClient wifi_client;
EthernetClient ethernet_client;
PubSubClient mqtt;

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
  led_strips[led.strip][led.led] = CRGB(led.r, led.g, led.b);
  FastLED.show();
  delay(1);
  xSemaphoreGive(led_mtx);

  char payload[64];
  sprintf(payload, "{\"color\":{\"r\":%u,\"g\":%u,\"b\":%u}}", led.r, led.g,
          led.b);
  mqtt.publish(topic, payload, true);

  vTaskDelay(led.timeout * 1000 / portTICK_PERIOD_MS);

  xSemaphoreTake(led_mtx, portMAX_DELAY);
  led_strips[led.strip][led.led] = CRGB(0, 0, 0);
  FastLED.show();
  delay(1);
  xSemaphoreGive(led_mtx);

  mqtt.publish(topic, LED_OFF_PAYLOAD, true);
  log_d("Turned strip %u led %u off", led.strip, led.led);

  vTaskDelete(NULL);
}

/**
 * @brief Turns the LEDs of all LED strips sequentially in a fading sequence
 *
 * @param group_size number of LEDs to be lit at each time
 */
void LED_init_sequence(uint16_t group_size = 10) {
  xSemaphoreTake(led_mtx, portMAX_DELAY);
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    for (uint8_t i = 0; i < NUM_STRIPS; i++) {
      led_strips[i][j] = CRGB(255, 255, 255);
    }
    if (j - group_size >= 0) {
      for (uint8_t i = 0; i < NUM_STRIPS; i++) {
        led_strips[i][j - group_size] = CRGB(0, 0, 0);
      }
      FastLED.show();
    }
    log_d("Lighting up LED # %d", j);
  }
  xSemaphoreGive(led_mtx);
}

/**
 * @brief Sets the first LED of all LED strips to signal the link status.
 * It turns green if a Ethernet connection is active, blue if Ethernet is not
 * available and WiFi is available otherwise the LEDs are off
 *
 * @param wifi_on state of WiFi connection
 * @param ethernet_on state of Ethernet connection
 */
void LED_link_state(bool wifi_on, bool ethernet_on) {
  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    led_strips[i][0] = ethernet_on ? CRGB(0, 255, 0)
                       : wifi_on   ? CRGB(0, 0, 255)
                                   : CRGB(0, 0, 0);
  }
  FastLED.show();
}

void mqtt_receive(char *topic, byte *payload, unsigned int length) {
  log_d("Message arrived [%s]: %s", topic, strndup((char *)payload, length));

  // skip prefix, device ID and forward slash
  topic += strlen(MQTT_TOPIC_PREFIX) + 10 + 1;

  log_d("Trimmed topic to %s", topic);

  char *token = strtok(topic, "/");
  if (strcmp(token, "multiple") == 0) {

    if (strcmp((char *)payload, "all") == 0) {
      for (uint16_t j = 0; j < NUM_LEDS; j++) {
        for (uint8_t i = 0; i < NUM_STRIPS; i++) {
          led_ctrl led;
          led.strip = i;
          led.led = j;
          led.r = 255;
          led.g = 255;
          led.b = 255;
          led.timeout = 1;

          xTaskCreate(control_LED, "control_LED", 2048, (void *)&led, 1, NULL);
        }
        delay(1000);
      }
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
      }
    }
  } else if (strcmp(token, "test") == 0) {
    LED_init_sequence();
  } else {
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
  }
  delay(1);
}

bool mqtt_connect() {
  if (mqtt.connected())
    return true;

  bool status = mqtt.connect(device_id, MQTT_USER, MQTT_PASS);

  if (status == false) {
    return false;
  }
  char topic[64];
  sprintf(topic, "%s%s/+/+", MQTT_TOPIC_PREFIX, device_id);
  mqtt.subscribe(topic, 1);
  log_v("Subscribed to topic: %s", topic);

  sprintf(topic, "%s%s/test", MQTT_TOPIC_PREFIX, device_id);
  mqtt.subscribe(topic);
  log_v("Subscribed to topic: %s", topic);

  sprintf(topic, "%s%s/multiple", MQTT_TOPIC_PREFIX, device_id);
  mqtt.subscribe(topic, 1);
  log_v("Subscribed to topic: %s", topic);

  log_i("Connected to MQTT server: %s:%d", MQTT_SERVER, MQTT_PORT);

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

void setup() {
  FastLED.addLeds<WS2812, LED_STRIP_1_PIN, GRB>(led_strips[0], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_2_PIN, GRB>(led_strips[1], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_3_PIN, GRB>(led_strips[2], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_4_PIN, GRB>(led_strips[3], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_5_PIN, GRB>(led_strips[4], NUM_LEDS);
  FastLED.addLeds<WS2812, LED_STRIP_6_PIN, GRB>(led_strips[5], NUM_LEDS);

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

  Ethernet.init(5);
  Ethernet.begin(mac);

  WiFi.begin(WiFi_SSID, WiFi_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    log_i(".");
    delay(100);
  }

  delay(2000);

  if (ethernet_client.connected()) {
    mqtt.setClient(ethernet_client);
  } else {
    mqtt.setClient(wifi_client);
  }

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);

  mqtt.setCallback(mqtt_receive);

  xTaskCreate(mqtt_announce, "announce_ID", 2048, NULL, 1, NULL);
}

unsigned long last_link_refresh = 0;

void loop() {
  if (millis() - last_link_refresh > LINK_REFRESH_INTERVAL) {
    // Check if DHCP lease needs to be renewed
    Ethernet.maintain();

    // Check if WiFI connection needs to be reset
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }

    if (ethernet_client.connected()) {
      mqtt.setClient(ethernet_client);
    } else {
      mqtt.setClient(wifi_client);
    }

    LED_link_state(wifi_client.connected(), ethernet_client.connected());
    last_link_refresh = millis();
  }

  if (mqtt_connect()) {
    mqtt.loop();
  }
}
