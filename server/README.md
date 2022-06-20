# Warehouse Locator - Server stack

The server is simply a MQTT broker. To setup the server you need to install [Docker](https://docs.docker.com/get-docker/) and [Docker Compose](https://docs.docker.com/compose/install/).

Because this will be a publicly accessible MQT broker,before you run the server, you need to define its access credentials. For that create a file inside the folder ```mosquitto``` called ```mosquitto.passwd``` with your user and password separated by a colon, here is an example:
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