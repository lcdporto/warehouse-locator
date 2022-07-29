# Warehouse Locator - An LED based stock search helper
Identifying a specific stock items inside a dense environment, such as a warehouse, is difficult so we developed a LED strip based system that lights up around the stock item requested.

This system is connected via WiFi to a MQTT broker and is able to receive commands to light up any given set of LEDs.

## Set up the server
The server is simply a MQTT broker. To setup the server you need to install [Docker](https://docs.docker.com/get-docker/) and [Docker Compose](https://docs.docker.com/compose/install/).

Because this will be a publicly accessible MQT broker,before you run the server, you need to define its access credentials. For that create a file on ```server/mosquitto/mosquitto.passwd``` with your user and password separated by a colon, here is an example:
```
lcdporto:very-strong-password
```
Then you are ready to start the server:

```
docker-compose up -d
```
Before you can use it you need to encrypt the password file:
```
docker exec -it mosquitto mosquitto_passwd -U /mosquitto/config/mosquitto.passwd
```
Then restart your server with:
```
docker-compose restart mosquitto
```
Now your server is up and running!

## Prepare your device

The hardware is composed of a ESP32 and LED strips. The hardware connections are simple:
| ESP32  | LED strip |
| ------ | ---- |
| GPIO15 | DATA |
| GPIO16 | DATA |
| GPIO19 | DATA |
| GPIO22 | DATA |
| GPIO21 | DATA |
| GPIO25 | DATA |

You can change this pin assignment as well as add more LED strips on the firmware. Your limitation is the RAM usage and the pin count of your board.

The firmware was developed using PlatformIO which can be found on the ```firmware``` folder and used according to the [the official documentation.](https://platformio.org/platformio-ide). At the time of this writing the recommended method is to use [VS Code](https://code.visualstudio.com/) and the [official PlatformIO extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) which is a procedure that works on Windows, macOS and Linux.

However, you can also use a CLI approach that is also available on all major platforms with Python installed (follow [this guide](https://wiki.python.org/moin/BeginnersGuide/Download)) and then on the ```firmware``` run the following lines on the terminal of your operative system:

```
pip install --user platformio
pio run --target upload
```

**Note:** you may need to reboot your system between the two commands for the *pio* executable to be available.

To configure the device to use your mqtt broker power up your device and search for a WiFi network named after this project "warehouse-locator-\<ID\>". Connect to it and you should be promted to login to the network on a captive portal. There is where you find a form to fill in your WiFi credentials and MQTT server details.

Alternatively, you can upload a file containing the MQTT server details (```firmware/data/config.json```) to the ESP32 filesystem following the [this guide](https://randomnerdtutorials.com/esp32-vs-code-platformio-spiffs/). Keep in mind that you still need to follow the captive portal procedure to enter your WiFi credentials.

## Example MQTT payload

The device can be controlled by sending a MQTT message to the broker identifying the device, led-strip and led index on the topic name as shown here:

### `warehouse-locator/<deviceId>/<strip-id>/<led-id>`

```json
{
  "state": "on",
  "color": {
    "r": 0,
    "g": 255,
    "b": 0
  },
  "timeout": 50
}
```
You can find a python script we used in our demo at [firmware/test/demo_pub.py](./firmware/test/demo_pub.py) as an example to control a device with two led strips. 