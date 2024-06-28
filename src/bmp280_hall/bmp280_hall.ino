#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "WifiPassword.h"
#include "mqttCredentials.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
 
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)
 
Adafruit_BMP280 bme; // I2C
//Adafruit_BMP280 bme(BMP_CS); // hardware SPI
//Adafruit_BMP280 bme(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);

// MQTT Broker Configuration
const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint
const char *mqtt_topic_temperature = "home/hall/temperature";
const char *mqtt_topic_pressure = "home/hall/pressure";
const char *mqtt_topic_height = "home/hall/height";

const char *mqtt_username = username;  // MQTT username for authentication
const char *mqtt_password = password_mqtt;  // MQTT password for authentication
const int mqtt_port = 1883;  // MQTT port (TCP)

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void connectToWiFi();

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

 
void setup() {
  Serial.begin(9600);
  Serial.println(F("BMP280 test"));
 
  if (!bme.begin(0x76)) { 
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to the WiFi network");
}

void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");

            mqtt_client.subscribe(mqtt_topic_temperature);
            mqtt_client.subscribe(mqtt_topic_pressure);
            mqtt_client.subscribe(mqtt_topic_height);

            // Publish message upon successful connection
            // mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }

  mqtt_client.loop();
  // mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 from Lorenzo's hall");


  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");
  
  Serial.print("Pressure = ");
  Serial.print(bme.readPressure());
  Serial.println(" Pa");
  
  Serial.print("Approx altitude = ");
  Serial.print(bme.readAltitude(1013.25)); // this should be adjusted to your local forcase
  Serial.println(" m");
  
  Serial.println();

  mqtt_client.publish(mqtt_topic_pressure, String(bme.readPressure()).c_str());
  mqtt_client.publish(mqtt_topic_temperature, String(bme.readTemperature()).c_str());
  mqtt_client.publish(mqtt_topic_height, String(bme.readAltitude(SEALEVELPRESSURE_HPA)).c_str());


  delay(10000);
}