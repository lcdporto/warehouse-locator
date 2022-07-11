# Warehouse Locator - An LED based stock search helper
Identifying a specific stock items inside a dense environment, such as a warehouse, is difficult so we developed a LED strip based system that lights up around the stock item requested.

This system is connected via WiFi to a MQTT broker and is able to receive commands to light up any given set of LEDs.

## Server
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

# Firmware

The firmware was developed using PlatformIO which can be found on the ```firmware``` folder and used according to the [the official documentation.](https://docs.platformio.org/en/stable/core/quickstart.html)

## Example MQTT payloads

### `warehouse-locator/<deviceId>/<strip-id>/<led-id>`

```json
on
```
or
```json
off
```