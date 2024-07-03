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
uint8_t MAX_FORECASTS = 2;

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
      answer = "So you need _help_, uh? me too! use /commands or /status";
    
    else if (msg.text == "/commands")
      answer = "Welcome my new friend! You are the first *" + msg.from_name + "* I've ever met.\nHere's the command you can use to check the weather: /todayweather and /forecastweather";
    
    else if (msg.text == "/status")
      answer = "All is good here, thanks for asking!";
    
    else if (msg.text == "/todayweather"){
      OpenWeatherMapCurrentData data;
      client.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
      client.setMetric(IS_METRIC);
      client.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);

      String msg_today = "*Forecast Number* " + String(i) + "\n" + "Temperature: " + String(data.temp) +"\n" 
      + "Minimum Temperature: " + String(data.tempMin) + "\n" + "Maximum Temperature: " + String(data.tempMax) +
      "\n" + "Humidity: " + String(data.humidity) + "\n" + "Description: " + String(data.description) + "\n";

      bot.sendMessage(msg.chat_id, msg_today, "Markdown");

    } else if (msg.text == "/forecastweather"){
      
      OpenWeatherMapForecastData data[MAX_FORECASTS];
      client_forecast.setMetric(IS_METRIC);
      client_forecast.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
      uint8_t allowedHours[] = {0,12};
      client_forecast.setAllowedHours(allowedHours, 2);
      uint8_t foundForecasts = client_forecast.updateForecastsById(data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
      const char* description_forecast[foundForecasts];

      for (uint8_t i = 0; i < foundForecasts; i++) {
          String msg_forecast = "*Forecast Number* " + String(i) + "\n" + "Temperature: " + String(data[i].temp) +"\n" 
          + "Minimum Temperature: " + String(data[i].tempMin) + "\n" + "Maximum Temperature: " + String(data[i].tempMax) +
          "\n" + "Humidity: " + String(data[i].humidity) + "\n" + "Description: " + String(data[i].description) + "\n";
          bot.sendMessage(msg.chat_id, msg_forecast, "Markdown");
      }
    } else
      answer = "Say what?";

    bot.sendMessage(msg.chat_id, answer, "Markdown");
  }
}

void bot_setup()
{
  const String commands = F("["
                            "{\"command\":\"todayweather\",\"description\":\"Shows Today's Weather\"},"
                            "{\"command\":\"forecastweather\",\"description\":\"Shows Forecast Weather up to 15 days\"},"
                            "{\"command\":\"commands\", \"description\":\"Message sent when you open a chat with a bot\"},"
                            "{\"command\":\"status\",\"description\":\"Answer device current status\"},"
                            "{\"command\":\"help\",  \"description\":\"Get bot usage help\"}" // no comma on last command
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
