#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <JsonListener.h>
#include <time.h>
#include "OpenWeatherMapForecast.h"
#include <PubSubClient.h>
#include "mqttCredentials.h"
#include "WifiPassword.h"
#include "openweather_api_key.h"
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "telegrambotCredentials.h"

// #define BOTtoken "7252474532:AAHUlYQnSZt9hoN_IlRK7RgqY0zQRg3oEtM"
// #define CHAT_ID "33523022"

const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint
const char *mqtt_topic_temperature = "home/hall/temperature";
const char *mqtt_topic_pressure = "home/hall/pressure";
const char *mqtt_topic_height = "home/hall/height";
const char *mqtt_topic_weather = "weather/";

const char *mqtt_username = username;  // MQTT username for authentication
const char *mqtt_password = password_mqtt;  // MQTT password for authentication
const int mqtt_port = 1883;  // MQTT port (TCP)
const char BotToken[] = BotToken;
const char ChatID[] = ChatID;
// BMP280 Sensor Settings
#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP280 bme;

// initiate the client
OpenWeatherMapForecast client;
String OPEN_WEATHER_MAP_APP_ID = open_weather_api_key;
String OPEN_WEATHER_MAP_LOCATION_ID = "3181903";
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = false;
uint8_t MAX_FORECASTS = 3;
bool rain_notification = false;

/**
 * WiFi Settings
 */
#if defined(ESP8266)
const char* ESP_HOST_NAME = "esp-" + ESP.getFlashChipId();
#else
const char* ESP_HOST_NAME = "esp-" + ESP.getEfuseMac();
#endif
const char* WIFI_SSID     = ssid;
const char* WIFI_PASSWORD = password;

// initiate the WifiClient
WiFiClient wifiClient;
WiFiClientSecure wifiClientSecure;
PubSubClient mqtt_client(wifiClient);

// X509List cert(TELEGRAM_CERTIFICATE_ROOT);
UniversalTelegramBot bot(BotToken, wifiClientSecure); 

/**
 * SETUP
 */
void setup() {
  // Connecting to wifi
  Serial.begin(115200);
  delay(500);
  connectWifi();

  //configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  //wifiClient.setTrustAnchors(&cert); // Add root certificate for api.telegram.org

  // Starting mqtt
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  // Checking if the sensor is connected properly
  if (!bme.begin(0x76)) { 
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  wifiClientSecure.setInsecure();

  Serial.println();
}


/**
 * Helping funtions
 */
void connectWifi() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());
  Serial.println();
}


void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-weather-station " + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic_weather);
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

/**
 * LOOP
 */
void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }

  mqtt_client.loop();

  Serial.println("\n\nNext Loop-Step: " + String(millis()) + ":");

  OpenWeatherMapForecastData data[MAX_FORECASTS];
  client.setMetric(IS_METRIC);
  client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {0,12};
  client.setAllowedHours(allowedHours, 2);
  uint8_t foundForecasts = client.updateForecastsById(data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
  Serial.printf("Found %d forecasts in this call\n", foundForecasts);
  Serial.println("------------------------------------");
  time_t time;
  const char* description_forecast[foundForecasts];
  for (uint8_t i = 0; i < foundForecasts; i++) {
    Serial.printf("---\nForecast number: %d\n", i);
    // {"dt":1527066000, uint32_t observationTime;
    time = data[i].observationTime;
    Serial.printf("observationTime: %d, full date: %s", data[i].observationTime, ctime(&time));
    // "main":{
    //   "temp":17.35, float temp;
    Serial.printf("temp: %f\n", data[i].temp);
    //   "feels_like": 16.99, float feelsLike;
    Serial.printf("feels-like temp: %f\n", data[i].feelsLike);
    //   "temp_min":16.89, float tempMin;
    Serial.printf("tempMin: %f\n", data[i].tempMin);
    //   "temp_max":17.35, float tempMax;
    Serial.printf("tempMax: %f\n", data[i].tempMax);
    //   "pressure":970.8, float pressure;
    Serial.printf("pressure: %f\n", data[i].pressure);
    //   "sea_level":1030.62, float pressureSeaLevel;
    Serial.printf("pressureSeaLevel: %f\n", data[i].pressureSeaLevel);
    //   "grnd_level":970.8, float pressureGroundLevel;
    Serial.printf("pressureGroundLevel: %f\n", data[i].pressureGroundLevel);
    //   "humidity":97, uint8_t humidity;
    Serial.printf("humidity: %d\n", data[i].humidity);
    //   "temp_kf":0.46
    // },"weather":[{
    //   "id":802, uint16_t weatherId;
    Serial.printf("weatherId: %d\n", data[i].weatherId);
    //   "main":"Clouds", String main;
    Serial.printf("main: %s\n", data[i].main.c_str());
    //   "description":"scattered clouds", String description;
    Serial.printf("description: %s\n", data[i].description.c_str());
    description_forecast[i] = data[i].description.c_str();

    //   "icon":"03d" String icon; String iconMeteoCon;
    Serial.printf("icon: %s\n", data[i].icon.c_str());
    Serial.printf("iconMeteoCon: %s\n", data[i].iconMeteoCon.c_str());
    // }],"clouds":{"all":44}, uint8_t clouds;
    Serial.printf("clouds: %d\n", data[i].clouds);
    // "wind":{
    //   "speed":1.77, float windSpeed;
    Serial.printf("windSpeed: %f\n", data[i].windSpeed);
    //   "deg":207.501 float windDeg;
    Serial.printf("windDeg: %f\n", data[i].windDeg);
    // rain: {3h: 0.055}, float rain;
    Serial.printf("rain: %f\n", data[i].rain);
    // },"sys":{"pod":"d"}
    // dt_txt: "2018-05-23 09:00:00"   String observationTimeText;
    Serial.printf("observationTimeText: %s\n", data[i].observationTimeText.c_str());
  }

  mqtt_client.publish(mqtt_topic_weather, description_forecast[0]);

  if (strstr(description_forecast[0], "rain") != NULL && rain_notification == false) {
      bot.sendMessage(ChatID, "Hey I just wanted to tell you that tomorrow is going to rain, so the air will be better! :3","");
      rain_notification = true;   
  } else {
    rain_notification = false;
  }

  Serial.println();
  Serial.println("---------------------------------------------------/\n");


  Serial.print("Hall Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");
  
  Serial.print("Hall Pressure = ");
  Serial.print(bme.readPressure());
  Serial.println(" Pa");
  
  Serial.print("Hall Approx altitude = ");
  Serial.print(bme.readAltitude(1013.25)); // this should be adjusted to your local forcase
  Serial.println(" m");
  
  Serial.println();
  Serial.println("---------------------------------------------------/\n");

  Serial.println();
  mqtt_client.publish(mqtt_topic_pressure, String(bme.readPressure()).c_str());
  mqtt_client.publish(mqtt_topic_temperature, String(bme.readTemperature()).c_str());
  mqtt_client.publish(mqtt_topic_height, String(bme.readAltitude(SEALEVELPRESSURE_HPA)).c_str());

  delay(10000);
}
