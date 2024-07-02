#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <JsonListener.h>
#include "OpenWeatherMapForecast.h"
#include "OpenWeatherMapCurrent.h"
#include "WifiPassword.h"
#include "openweather_api_key.h"
#include "telegrambotCredentials.h"

// Wifi network station credentials
#define WIFI_SSID ssid
#define WIFI_PASSWORD password
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN BotToken

OpenWeatherMapCurrent client;
OpenWeatherMapForecast client_forecast;

String OPEN_WEATHER_MAP_APP_ID = open_weather_api_key;
String OPEN_WEATHER_MAP_LOCATION_ID = "3181903";
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = true;
uint8_t MAX_FORECASTS = 15;

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

unsigned long bot_lasttime; // last time messages' scan has been done
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void handleNewMessages(int numNewMessages)
{
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);
  
  String answer;
  for (int i = 0; i < numNewMessages; i++)
  {
    telegramMessage &msg = bot.messages[i];
    Serial.println("Received " + msg.text);
   
    if (msg.text == "/help")
      answer = "So you need _help_, uh? me too! use /start or /status";
    
    else if (msg.text == "/start")
      answer = "Welcome my new friend! You are the first *" + msg.from_name + "* I've ever met\n. Here's the command you can use to check the weather: /TodayWeather and /ForecastWeather";
    
    else if (msg.text == "/status")
      answer = "All is good here, thanks for asking!";
    
    else if (msg.text == "/todayweather"){
      OpenWeatherMapCurrentData data;
      client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
      client.setMetric(IS_METRIC);
      client.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
      Serial.printf("lon: %f\n", data.lon);
      // "lat": 47.37 float lat;
      Serial.printf("lat: %f\n", data.lat);
      // "id": 521, weatherId weatherId;
      Serial.printf("weatherId: %d\n", data.weatherId);
      // "main": "Rain", String main;
      Serial.printf("main: %s\n", data.main.c_str());
      // "description": "shower rain", String description;
      Serial.printf("description: %s\n", data.description.c_str());
      // "icon": "09d" String icon; String iconMeteoCon;
      Serial.printf("icon: %s\n", data.icon.c_str());
      Serial.printf("iconMeteoCon: %s\n", data.iconMeteoCon.c_str());
      // "temp": 290.56, float temp;
      Serial.printf("temp: %f\n", data.temp);
      // "pressure": 1013, uint16_t pressure;
      Serial.printf("pressure: %d\n", data.pressure);
      // "humidity": 87, uint8_t humidity;
      Serial.printf("humidity: %d\n", data.humidity);
      // "temp_min": 289.15, float tempMin;
      Serial.printf("tempMin: %f\n", data.tempMin);
      // "temp_max": 292.15 float tempMax;
      Serial.printf("tempMax: %f\n", data.tempMax);
      // "wind": {"speed": 1.5}, float windSpeed;
      Serial.printf("windSpeed: %f\n", data.windSpeed);
      // "wind": {"deg": 1.5}, float windDeg;
      Serial.printf("windDeg: %f\n", data.windDeg);
      // "clouds": {"all": 90}, uint8_t clouds;
      Serial.printf("clouds: %d\n", data.clouds);
      // "dt": 1527015000, uint64_t observationTime;
      time_t time = data.observationTime;
      Serial.printf("observationTime: %d, full date: %s", data.observationTime, ctime(&time));
      // "country": "CH", String country;
      Serial.printf("country: %s\n", data.country.c_str());
      // "sunrise": 1526960448, uint32_t sunrise;
      time = data.sunrise;
      Serial.printf("sunrise: %d, full date: %s", data.sunrise, ctime(&time));
      // "sunset": 1527015901 uint32_t sunset;
      time = data.sunset;
      Serial.printf("sunset: %d, full date: %s", data.sunset, ctime(&time));

      // "name": "Zurich", String cityName;
      Serial.printf("cityName: %s\n", data.cityName.c_str());
      Serial.println();
      Serial.println("---------------------------------------------------/\n");

    } else if (msg.text == "/forecastweather"){
      
      OpenWeatherMapForecastData data[MAX_FORECASTS];
      client_forecast.setMetric(IS_METRIC);
      client_forecast.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
      uint8_t allowedHours[] = {0,12};
      client_forecast.setAllowedHours(allowedHours, 2);
      uint8_t foundForecasts = client_forecast.updateForecastsById(data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
      
      // Serial.printf("Found %d forecasts in this call\n", foundForecasts);
      // Serial.println("------------------------------------");
      //time_t time;
      const char* description_forecast[foundForecasts];
      for (uint8_t i = 0; i < foundForecasts; i++) {
          String msg_forecast = "---------------------\nForecast Number " + String(i);
          // msg_forecast= "-------------------\nForecast number: " + String(i) + "\n" + "temp: " + String(data[i].temp) +"\n" 
          // + "tempMin: " + String(data[i].tempMin) + "tempMax: " + String(data[i].tempMax) +
          // "\n" + "humidity " + String(data[i].humidity) + "\n" + "desctiption: " + String(data[i].description) + "\n";
          //Serial.printf("---\nForecast number: %d\n", i);
          bot.sendMessage(msg.chat_id, msg_forecast, "Markdown");
      }

          // {"dt":1527066000, uint32_t observationTime;
          //time = data[i].observationTime;
          //Serial.printf("observationTime: %d, full date: %s", data[i].observationTime, ctime(&time));
          // "main":{
          //   "temp":17.35, float temp;
          //Serial.printf("temp: %f\n", data[i].temp);
          //   "feels_like": 16.99, float feelsLike;
          //Serial.printf("feels-like temp: %f\n", data[i].feelsLike);
          //   "temp_min":16.89, float tempMin;
          //Serial.printf("tempMin: %f\n", data[i].tempMin);
          //   "temp_max":17.35, float tempMax;
          //Serial.printf("tempMax: %f\n", data[i].tempMax);
          //   "pressure":970.8, float pressure;
          //Serial.printf("pressure: %f\n", data[i].pressure);
          //   "sea_level":1030.62, float pressureSeaLevel;
          //Serial.printf("pressureSeaLevel: %f\n", data[i].pressureSeaLevel);
          //   "grnd_level":970.8, float pressureGroundLevel;
          //Serial.printf("pressureGroundLevel: %f\n", data[i].pressureGroundLevel);
          //   "humidity":97, uint8_t humidity;
          //Serial.printf("humidity: %d\n", data[i].humidity);
          //   "temp_kf":0.46
          // },"weather":[{
          //   "id":802, uint16_t weatherId;
          //Serial.printf("weatherId: %d\n", data[i].weatherId);
          //   "main":"Clouds", String main;
          //Serial.printf("main: %s\n", data[i].main.c_str());
          //   "description":"scattered clouds", String description;
          //Serial.printf("description: %s\n", data[i].description.c_str());
          //   "icon":"03d" String icon; String iconMeteoCon;
          //Serial.printf("icon: %s\n", data[i].icon.c_str());
          //Serial.printf("iconMeteoCon: %s\n", data[i].iconMeteoCon.c_str());
          // }],"clouds":{"all":44}, uint8_t clouds;
          //Serial.printf("clouds: %d\n", data[i].clouds);
          // "wind":{
          //   "speed":1.77, float windSpeed;
          //Serial.printf("windSpeed: %f\n", data[i].windSpeed);
          //   "deg":207.501 float windDeg;
          //Serial.printf("windDeg: %f\n", data[i].windDeg);
          // rain: {3h: 0.055}, float rain;
          //Serial.printf("rain: %f\n", data[i].rain);
          // },"sys":{"pod":"d"}
          // dt_txt: "2018-05-23 09:00:00"   String observationTimeText;
          //Serial.printf("observationTimeText: %s\n", data[i].observationTimeText.c_str());
      //} 
    } else
      answer = "Say what?";

    bot.sendMessage(msg.chat_id, answer, "Markdown");
  }
}

void bot_setup()
{
  const String commands = F("["
                            "{\"command\":\"help\",  \"description\":\"Get bot usage help\"},"
                            "{\"command\":\"start\", \"description\":\"Message sent when you open a chat with a bot\"},"
                            "{\"command\":\"status\",\"description\":\"Answer device current status\"},"
                            "{\"command\":\"todayweather\",\"description\":\"Shows Today's Weather\"},"
                            "{\"command\":\"forecastweather\",\"description\":\"Shows Forecast Weather up to 15 days\"}" // no comma on last command
                            "]");

  bot.setMyCommands(commands);
  //bot.sendMessage("25235518", "Hola amigo!", "Markdown");
}

void setup()
{
  Serial.begin(9600);
  Serial.println();

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Check NTP/Time, usually it is instantaneous and you can delete the code below.
  Serial.print("Retrieving time: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  bot_setup();
}

void loop()
{
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}
