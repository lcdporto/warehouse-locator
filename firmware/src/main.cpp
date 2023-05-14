#include <ArduinoJson.h>
#include <EthernetENC.h>
#include <FastLED.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <WiFi.h>

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

char device_id[11] = "";

static CRGB led_strips[NUM_STRIPS][NUM_LEDS];

EthernetClient ethernet_client;
PubSubClient mqtt(ethernet_client);

static bool test_running = false;

typedef struct {
  uint8_t strip;
  uint16_t led[128];
  uint8_t n_leds;
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;
  uint16_t timeout = 5;
} led_ctrl;

void control_LEDs(void *led_arg) {
  led_ctrl led;
  memcpy(&led, led_arg, sizeof(led_ctrl));

  for (uint i = 0; i < led.n_leds; i++) {
    led_strips[led.strip][led.led[i]] = CRGB(led.r, led.g, led.b);
  }
  FastLED.show();

  for (uint i = 0; i < led.n_leds; i++) {
  char topic[64];
  char payload[64];
    sprintf(topic, "%s%s/%d/%d/state", MQTT_TOPIC_PREFIX, device_id, led.strip,
            led.led[i]);
  sprintf(payload, "{\"color\":{\"r\":%u,\"g\":%u,\"b\":%u}}", led.r, led.g,
          led.b);
    log_d("Publishing to state topic: %s", topic);
  mqtt.publish(topic, payload, true);
  }

  vTaskDelay((led.timeout * 1000) / portTICK_PERIOD_MS);

  for (uint i = 0; i < led.n_leds; i++) {
    led_strips[led.strip][led.led[i]] = CRGB(0, 0, 0);
  }
  FastLED.show();

  log_d("Turned strip %u led %u off", led.strip, led.led);
  for (uint i = 0; i < led.n_leds; i++) {
    char topic[64];
    sprintf(topic, "%s%s/%d/%d/state", MQTT_TOPIC_PREFIX, device_id, led.strip,
            led.led[i]);
    sprintf(topic, "%s%s/%d/%d/state", MQTT_TOPIC_PREFIX, device_id, led.strip,
            led.led[i]);
    mqtt.publish(topic, LED_OFF_PAYLOAD, true);
  }

  vTaskDelete(NULL);
}

/**
 * @brief Turns the LEDs of all LED strips sequentially in a fading sequence
 *
 * @param group_size number of LEDs to be lit at each time
 */
void LED_init_sequence(uint16_t group_size = 3) {
  test_running = true;
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    for (uint8_t i = 0; i < NUM_STRIPS; i++) {
      led_strips[i][j] = CRGB(255, 255, 255);
    }
    if (j - group_size >= 0) {
      for (uint8_t i = 0; i < NUM_STRIPS; i++) {
        led_strips[i][j - group_size] = CRGB(0, 0, 0);
      }
    }
      FastLED.show();
    log_d("Lighting up LED # %d", j);
  }
  test_running = false;
}

/**
 * @brief Sets the first LED of all LED strips to signal the link status.
 * It turns green if a Ethernet connection is active, blue if Ethernet is not
 * available and WiFi is available otherwise the LEDs are off
 */
void LED_link_state(void *) {
  while (1) {
  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
      led_strips[i][0] =
          Ethernet.linkStatus() == LinkON && !test_running
              ? mqtt.connected() ? CRGB(0, 64, 0) : CRGB(0, 0, 64)
                                   : CRGB(0, 0, 0);
  }
  FastLED.show();
    vTaskDelay(2 * 1000 / portTICK_PERIOD_MS);
  }
}

void mqtt_receive(char *topic, byte *payload, unsigned int length) {
  log_d("Message arrived [%s]: %s", topic, strndup((char *)payload, length));

  // skip prefix, device ID and forward slash
  topic += strlen(MQTT_TOPIC_PREFIX) + 10 + 1;

  log_d("Trimmed topic to %s", topic);

  char *token = strtok(topic, "/");
  if (strcmp(token, "multiple") == 0) {

    DynamicJsonDocument doc(2048);
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

      log_d("Received command to control %d LEDs", doc["leds"].size());

      led_ctrl led_c;
      led_c.strip = doc["strip"];
      for (uint8_t i = 0; i < (uint16_t)doc["to"] - (uint16_t)doc["from"] + 1;
           i++) {
        led_c.led[i] = (uint16_t)doc["from"] + i;
      }
      led_c.n_leds = (uint8_t)doc["to"] - (uint8_t)doc["from"] + 1;
      led_c.r = doc["color"]["r"];
      led_c.g = doc["color"]["g"];
      led_c.b = doc["color"]["b"];
      led_c.timeout = timeout;

      xTaskCreate(control_LEDs, "control_LEDs", 4096, &led_c, 1, NULL);
    }
  } else if (strcmp(token, "test") == 0) {
    LED_init_sequence();
  } else {
    uint8_t strip_n;
    uint16_t led_n;

    strip_n = atoi(token);
    led_n = atoi(strtok(NULL, "/"));

    log_d("Addressed strip %u and led %u", strip_n, led_n);

    DynamicJsonDocument doc(128);
    if (deserializeJson(doc, (char *)payload)) {
      log_e("Received invalid JSON");
      return;
    }
    log_d("Deserialized JSON successfully");
    log_d("Doc has size: %d", doc.size());

    uint16_t timeout;
    if (doc.containsKey("timeout")) {
      timeout = doc["timeout"];
    } else {
      timeout = 5;
    }

    led_ctrl led_c;
    led_c.strip = strip_n;
    led_c.led[0] = led_n;
    led_c.n_leds = 1;
    led_c.r = doc["color"]["r"];
    led_c.g = doc["color"]["g"];
    led_c.b = doc["color"]["b"];
    led_c.timeout = timeout;

    xTaskCreate(control_LEDs, "control_LEDs", 4096, &led_c, 1, NULL);
  }
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

    for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    for (uint16_t j = 0; j < NUM_LEDS; j++) {
      led_strips[i][j] = CRGB(0, 0, 0);
    }
  }
  FastLED.show();

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

  log_i("Device ID: %s", device_id);

  Ethernet.init(5);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Ethernet.begin(mac);
  delay(1500);
  log_i("Connecting with mac address: %s", WiFi.macAddress().c_str());

  log_d("Configuring MQTT server");
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);

  log_d("Configuring MQTT callback");
  mqtt.setCallback(mqtt_receive);

  log_d("Setting up keep alive MQTT message");
  xTaskCreate(mqtt_announce, "announce_ID", 2048, NULL, 1, NULL);

  log_d("Setting up status LED");
  xTaskCreate(LED_link_state, "link_state", 2048, NULL, 1, NULL);
}

void loop() {
  if (mqtt_connect()) {
    mqtt.loop();
  }
}
