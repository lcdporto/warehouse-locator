import paho.mqtt.client as mqtt
import json
import time

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("$SYS/#")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.username_pw_set("lcd", password="lcd")
client.connect("localhost", 1883, 60)

on_white = {
  "state": "on",
  "color": {
    "r": 255,
    "g": 255,
    "b": 255
  },
  "timeout": 50
}

A1 = ["warehouse-locator/1952661117/0/0",
"warehouse-locator/1952661117/0/4",
"warehouse-locator/1952661117/1/3",
"warehouse-locator/1952661117/1/16"]

A2 = ["warehouse-locator/1952661117/0/1",
"warehouse-locator/1952661117/0/8",
"warehouse-locator/1952661117/1/2",
"warehouse-locator/1952661117/1/12"]

A3 = ["warehouse-locator/1952661117/0/3",
"warehouse-locator/1952661117/0/13",
"warehouse-locator/1952661117/1/0",
"warehouse-locator/1952661117/1/7"]

A4 = ["warehouse-locator/1952661117/0/2",
"warehouse-locator/1952661117/0/10",
"warehouse-locator/1952661117/1/1",
"warehouse-locator/1952661117/1/10"]

for a in A1:
    on_white["state"] = "on"
    client.publish(a, json.dumps(on_white))
time.sleep(5)
for a in A1:
    on_white["state"] = "off"
    client.publish(a, json.dumps(on_white))

time.sleep(3)

for a in A2:
    on_white["state"] = "on"
    client.publish(a, json.dumps(on_white))
time.sleep(5)
for a in A2:
    on_white["state"] = "off"
    client.publish(a, json.dumps(on_white))

time.sleep(3)

for a in A3:
    on_white["state"] = "on"
    client.publish(a, json.dumps(on_white))
time.sleep(2)
for a in A4:
    on_white["state"] = "on"
    on_white["color"]["r"] = 255
    on_white["color"]["g"] = 0
    on_white["color"]["b"] = 0
    client.publish(a, json.dumps(on_white))
time.sleep(2)
for a in A2:
    on_white["state"] = "on"
    on_white["color"]["r"] = 0
    on_white["color"]["g"] = 255
    on_white["color"]["b"] = 0
    client.publish(a, json.dumps(on_white))
time.sleep(2)
for a in A1:
    on_white["state"] = "on"
    on_white["color"]["r"] = 0
    on_white["color"]["g"] = 0
    on_white["color"]["b"] = 255
    client.publish(a, json.dumps(on_white))
time.sleep(8)
for a in A1:
    on_white["state"] = "off"
    client.publish(a, json.dumps(on_white))
for a in A2:
    on_white["state"] = "off"
    client.publish(a, json.dumps(on_white))
for a in A3:
    on_white["state"] = "off"
    client.publish(a, json.dumps(on_white))
for a in A4:
    on_white["state"] = "off"
    client.publish(a, json.dumps(on_white))

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
#client.loop_forever()