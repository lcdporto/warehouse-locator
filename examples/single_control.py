import paho.mqtt.client as mqtt
import json
import time

MQTT_TOPIC_PREFIX = "warehouse-locator/"
MQTT_SERVER_ADDRESS = "localhost"
MQTT_SERVER_PORT = 1883
MQTT_USER = "lcdporto"
MQTT_PASSWORD = "very-strong-password"
DEVICE_ID = "1952661117"


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))


client = mqtt.Client()
client.on_connect = on_connect

client.username_pw_set(MQTT_USER, password=MQTT_PASSWORD)
client.connect(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT, 60)

white = {
    "color": {
        "r": 255,
        "g": 255,
        "b": 255
    },
    "timeout": 5
}

red = {
    "color": {
        "r": 255,
        "g": 0,
        "b": 0
    },
    "timeout": 5
}

green = {
    "color": {
        "r": 0,
        "g": 255,
        "b": 0
    },
    "timeout": 5
}

blue = {
    "color": {
        "r": 0,
        "g": 0,
        "b": 255
    },
    "timeout": 5
}

A1 = [MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/0",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/4",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/3",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/16"]

A2 = [MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/1",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/8",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/2",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/12"]

A3 = [MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/3",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/13",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/0",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/7"]

A4 = [MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/2",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/0/10",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/1",
      MQTT_TOPIC_PREFIX+DEVICE_ID+"/1/10"]

for a in A1:
    client.publish(a, json.dumps(white))

time.sleep(3)

for a in A2:
    client.publish(a, json.dumps(white))

time.sleep(3)

for a in A3:
    white.timeout = 8
    client.publish(a, json.dumps(white))
time.sleep(2)
for a in A4:
    red.timeout = 8
    client.publish(a, json.dumps(red))
time.sleep(2)
for a in A2:
    green.timeout = 8
    client.publish(a, json.dumps(green))
time.sleep(2)
for a in A1:
    blue.timeout = 8
    client.publish(a, json.dumps(blue))
