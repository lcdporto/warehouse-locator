import paho.mqtt.client as mqtt
import json
import time

TOPIC_PREFIX = "warehouse-locator/"
MQTT_SERVER_ADDRESS = "localhost"
MQTT_SERVER_PORT = 1883
MQTT_USER = "lcdporto"
MQTT_PASSWORD = "very-strong-password"


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(TOPIC_PREFIX+"+")


def on_message(client, userdata, msg):
    print("Device found: " + msg.topic)


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set(MQTT_USER, password=MQTT_PASSWORD)
client.connect(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT, 60)

client.loop_forever()
