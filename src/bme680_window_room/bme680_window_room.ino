/***
  Read Our Complete Guide: https://RandomNerdTutorials.com/esp8266-nodemcu-bme680-sensor-arduino/
  Designed specifically to work with the Adafruit BME680 Breakout ----> http://www.adafruit.com/products/3660 These sensors use I2C or SPI to communicate, 2 or 4 pins are required to interface. Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit! Written by Limor Fried & Kevin Townsend for Adafruit Industries. BSD license, all text above must be included in any redistribution
***/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "WifiPassword.h"
#include "mqttCredentials.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <stdio.h> 

/*#define BME_SCK 14
#define BME_MISO 12
#define BME_MOSI 13
#define BME_CS 15*/

#define SEALEVELPRESSURE_HPA (1013.25)
#define MIN_VALUE 25
#define MAX_VALUE 800
#define REPORT_INTERVAL 1
#define MAX 100 

Adafruit_BME680 bme; // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

#define LIGHT_SENSOR_PIN A0 // The ESP8266 pin ADC0

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int   getgasreference_count = 0;

// MQTT Broker Configuration
const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint

// MQTT Topics
const char *mqtt_topic_temperature = "home/room/window/temperature";
const char *mqtt_topic_pressure = "home/room/window/pressure";
const char *mqtt_topic_humidity = "home/room/window/humidity";
const char *mqtt_topic_airquality = "home/room/window/airquality";
const char *mqtt_topic_light = "home/room/window/light";
const char *mqtt_topic_height = "home/room/window/height";
const char *mqtt_topic_extern = "extern/";

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
  while (!Serial);
  Serial.println(F("BME680 async test"));

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  GetGasReference();

  // MQTT Configuration
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
            mqtt_client.subscribe(mqtt_topic_humidity);
            mqtt_client.subscribe(mqtt_topic_airquality);
            mqtt_client.subscribe(mqtt_topic_light);
            mqtt_client.subscribe(mqtt_topic_height);
            mqtt_client.subscribe(mqtt_topic_extern);

            // Publish message upon successful connection
            // mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 Window Room ^^");
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

  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  Serial.print(F("Reading started at "));
  Serial.print(millis());
  Serial.print(F(" and will finish at "));
  Serial.println(endTime);

  Serial.println(F("You can do other work during BME680 measurement."));
  delay(50); // This represents parallel work.
  // There's no need to delay() until millis() >= endTime: bme.endReading()
  // takes care of that. It's okay for parallel work to take longer than
  // BME680's measurement time.

  // Obtain measurement results from BME680. Note that this operation isn't
  // instantaneous even if milli() >= endTime due to I2C/SPI latency.
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  Serial.print(F("Reading completed at "));
  Serial.println(millis());
  
  Serial.print(F("Temperature = "));
  Serial.print(bme.temperature);
  Serial.println(F(" *C"));

  Serial.print(F("Pressure = "));
  Serial.print(bme.pressure);
  Serial.println(F(" Pa"));

  Serial.print(F("Humidity = "));
  Serial.print(bme.humidity);
  Serial.println(F(" %"));

  Serial.print(F("Gas = "));
  Serial.print(bme.gas_resistance);
  Serial.println(F(" Ohms"));

  Serial.print(F("Approx. Altitude = "));
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(F(" m"));

  Serial.println();

  float current_humidity = bme.readHumidity();
  if (current_humidity >= 38 && current_humidity <= 42)
    hum_score = 0.25*100; // Humidity +/-5% around optimum 
  else
  { //sub-optimal
    if (current_humidity < 38) 
      hum_score = 0.25/hum_reference*current_humidity*100;
    else
    {
      hum_score = ((-0.25/(100-hum_reference)*current_humidity)+0.416666)*100;
    }
  }
  
  //Calculate gas contribution to IAQ index
  float gas_lower_limit = 5000;   // Bad air quality limit
  float gas_upper_limit = 50000;  // Good air quality limit 
  if (gas_reference > gas_upper_limit) gas_reference = gas_upper_limit;
  if (gas_reference < gas_lower_limit) gas_reference = gas_lower_limit;
  gas_score = (0.75/(gas_upper_limit-gas_lower_limit)*gas_reference -(gas_lower_limit*(0.75/(gas_upper_limit-gas_lower_limit))))*100;
  
  //Combine results for the final IAQ index value (0-100% where 100% is good quality air)
  float air_quality_score = hum_score + gas_score;

  Serial.println("Air Quality = "+String(air_quality_score,1)+"% derived from 25% of Humidity reading and 75% of Gas reading - 100% is good quality air");
  Serial.println("Humidity element was : "+String(hum_score/100)+" of 0.25");
  Serial.println("     Gas element was : "+String(gas_score/100)+" of 0.75");
  if (bme.readGas() < 120000) Serial.println("***** Poor air quality *****");
  Serial.println();
  if ((getgasreference_count++)%10==0) GetGasReference();
  String aqs = CalculateIAQ(air_quality_score);
  Serial.println(aqs);
  Serial.println("------------------------------------------------");
  

  int sensorValue = analogRead(LIGHT_SENSOR_PIN);
  float level = 100 - ((sensorValue - MIN_VALUE) * 100 / (MAX_VALUE - MIN_VALUE)); //normalised value
  
  sensorValue = level;
  Serial.println("LUX = ");
  Serial.println(sensorValue);


  // Light Data
  String light_text = "";
  if (sensorValue < 40) {
    Serial.println(" => Dark");
    light_text += "Dark";

  } else if (sensorValue < 60) {
    Serial.println(" => Dim");
    light_text += "Dim";

  } else if (sensorValue < 70) {
    Serial.println(" => Light");
    light_text += "Light";

  } else if (sensorValue < 80) {
    Serial.println(" => Bright");
    light_text += "Bright";

  } else {
    Serial.println(" => Very bright");
    light_text += "Very bright";

  }
  
  // External Condition Calculation
  String extern_message = "BAD";

  if (strcmp (aqs.c_str(), "Good") == 0 || strcmp (aqs.c_str(), "Moderate") == 0){
    if (bme.pressure > 100000) {
        if (bme.temperature > 12 && bme.temperature < 29){
          if  (bme.humidity < 60){
            extern_message = "GOOD";
          } else {
            extern_message = "MAYBE";
          }
        } else if (bme.temperature < 30 && bme.humidity < 60){
          extern_message = "MAYBE";
        }
    } else {
      extern_message = "BAD";
    }
  } else {
    extern_message = "BAD";
  }

  // MQTT Publish Messages
  mqtt_client.publish(mqtt_topic_airquality, aqs.c_str());
  mqtt_client.publish(mqtt_topic_light, light_text.c_str());
  mqtt_client.publish(mqtt_topic_pressure, String(bme.pressure).c_str());
  mqtt_client.publish(mqtt_topic_humidity, String(bme.humidity).c_str());
  mqtt_client.publish(mqtt_topic_temperature, String(bme.temperature).c_str());
  mqtt_client.publish(mqtt_topic_height, String(bme.readAltitude(SEALEVELPRESSURE_HPA)).c_str());
  mqtt_client.publish(mqtt_topic_extern, extern_message.c_str());


  delay(100);

  }

  void GetGasReference(){
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  Serial.println("Getting a new gas reference value");
  int readings = 10;
  for (int i = 1; i <= readings; i++){ // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
}

  String CalculateIAQ(float score){
  String IAQ_text = "";
  score = (100-score)*5;
  if      (score >= 301)                  IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderate";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Good";
  return IAQ_text;
}
