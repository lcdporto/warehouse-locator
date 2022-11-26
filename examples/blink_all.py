import paho.mqtt.client as mqtt
import json
import time

MQTT_TOPIC_PREFIX = "warehouse-locator/"
MQTT_SERVER_ADDRESS = "localhost"
MQTT_SERVER_PORT = 1883
MQTT_USER = "lcd"
MQTT_PASSWORD = "lcd"
DEVICE_ID = "4098283885"


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))


client = mqtt.Client()
client.on_connect = on_connect

client.username_pw_set(MQTT_USER, password=MQTT_PASSWORD)
client.connect(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT, 60)

topic = MQTT_TOPIC_PREFIX+DEVICE_ID+"/multiple"
client.publish(topic, "all")
