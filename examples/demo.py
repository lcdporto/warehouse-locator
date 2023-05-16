import paho.mqtt.client as mqtt
import json

MQTT_TOPIC_PREFIX = "warehouse-locator/"
MQTT_SERVER_ADDRESS = "test.mosquitto.org"
MQTT_SERVER_PORT = 1883
MQTT_USER = "user"
MQTT_PASSWORD = "password"

DEVICE_ID = "1234567890"

strip = 0 # 0-5
from_led = 1
to_led = 4

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))


client = mqtt.Client()
client.on_connect = on_connect

client.username_pw_set(MQTT_USER, password=MQTT_PASSWORD)
client.connect(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT, 60)

payload = {
    "strip": strip,
    "from": from_led,
    "to": to_led,
    "color": {
        "r": 255,
        "g": 255,
        "b": 255
    },
    "timeout": 1
}

client.publish(MQTT_TOPIC_PREFIX+DEVICE_ID+"/multiple", json.dumps(payload), 1)
