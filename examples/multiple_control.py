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

led_group = [
    {
        "strip": 0,
        "led": 0,
        "color": {
            "r": 255,
            "g": 0,
            "b": 0
        },
        "timeout": 5
    },
    {
        "strip": 1,
        "led": 0,
        "color": {
            "r": 0,
            "g": 255,
            "b": 0
        },
        "timeout": 5
    },
    {
        "strip": 2,
        "led": 0,
        "color": {
            "r": 0,
            "g": 0,
            "b": 255
        },
        "timeout": 5
    }
]

topic = MQTT_TOPIC_PREFIX+DEVICE_ID
client.publish(topic, json.dumps(led_group))
