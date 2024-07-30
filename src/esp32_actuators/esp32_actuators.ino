#include "pitches.h"
#include "WifiPassword.h"
#include "mqttCredentials.h"
#include <PubSubClient.h>
#include <WiFi.h>

#define BUZZER_PIN  5

#define PIN_RED    19 // GPIO23
#define PIN_GREEN  23 // GPIO22
#define PIN_BLUE   18 // GPIO21

int melody_danger[] = {
  NOTE_AS4,4,  NOTE_F4,-4, NOTE_AS4,4,  NOTE_F4,-4,
  NOTE_AS4,4,  NOTE_F4,-4, NOTE_AS4,4,  NOTE_F4,-4,
};

int melody_good[] = {

  NOTE_AS4,4,  NOTE_F4,-4,  NOTE_AS4,8,  NOTE_AS4,16,  NOTE_C5,16, NOTE_D5,16, NOTE_DS5,16,//7
  NOTE_F5,2,  NOTE_F5,8,  NOTE_F5,8,  NOTE_F5,8,  NOTE_FS5,16, NOTE_GS5,16,
  NOTE_AS5,-2,  NOTE_AS5,8,  NOTE_AS5,8,  NOTE_GS5,8,  NOTE_FS5,16,
  NOTE_GS5,-8,  NOTE_FS5,16,  NOTE_F5,2,  NOTE_F5,4, 

  NOTE_DS5,-8, NOTE_F5,16, NOTE_FS5,2, NOTE_F5,8, NOTE_DS5,8, //11
  NOTE_CS5,-8, NOTE_DS5,16, NOTE_F5,2, NOTE_DS5,8, NOTE_CS5,8,
  NOTE_C5,-8, NOTE_D5,16, NOTE_E5,2, NOTE_G5,8, 
  NOTE_F5,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,8, NOTE_F4,16,NOTE_F4,8,

};

int melody_maybe[] = {
  
  // Pacman
  // Score available at https://musescore.com/user/85429/scores/107109
  NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
  NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
  NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,

  NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
  NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
  NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8
  
};

const int melody_bad[] PROGMEM = {

  // At Doom's Gate (E1M1)
  // Score available at https://musescore.com/pieridot/doom

  NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8, //1
  NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
  NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
  NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, -2,

  NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8, //13
  NOTE_C3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_AS2, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_B2, 8, NOTE_C3, 8,
  NOTE_E2, 8, NOTE_E2, 8, NOTE_E3, 8, NOTE_E2, 8, NOTE_E2, 8, NOTE_D3, 8, NOTE_E2, 8, NOTE_E2, 8,
  NOTE_FS3, -16, NOTE_D3, -16, NOTE_B2, -16, NOTE_A3, -16, NOTE_FS3, -16, NOTE_B2, -16, NOTE_D3, -16, NOTE_FS3, -16, NOTE_A3, -16, NOTE_FS3, -16, NOTE_D3, -16, NOTE_B2, -16,
};

const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint

const char *mqtt_topic_temperature_room = "home/room/temperature";
const char *mqtt_topic_temperature_hall = "home/hall/temperature";
const char *mqtt_topic_actuators = "home/actuators";
const char *mqtt_topic_extern = "home/room/window/airquality";
const char *mqtt_topic_weather = "weather/";
const char *mqtt_topic_temperature_outside = "home/room/window/temperature";
const char *mqtt_topic_humidity_outside = "home/room/window/humidity";


const char *mqtt_username = username;  // MQTT username for authentication
const char *mqtt_password = password_mqtt;  // MQTT password for authentication
const int mqtt_port = 1883;  // MQTT port (TCP)

String mqttpayload_extern = "";
String mqttpayload_room_temp = "";
String mqttpayload_hall_temp = "";
String mqttpayload_weather = "";
String mqttpayload_temperature_outside = "";
String mqttpayload_humidity_outside = "";

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

bool good = 0;
bool maybe = 0;
bool bad = 0;

void connectToWiFi();

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
  Serial.begin(9600);

  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);

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
        String client_id = "esp32 actuators board " + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic_temperature_room);
            mqtt_client.subscribe(mqtt_topic_temperature_hall);
            mqtt_client.subscribe(mqtt_topic_actuators);
            mqtt_client.subscribe(mqtt_topic_extern);
            mqtt_client.subscribe(mqtt_topic_weather);
            mqtt_client.subscribe(mqtt_topic_temperature_outside);
            mqtt_client.subscribe(mqtt_topic_humidity_outside);


            
            // Publish message upon successful connection
            // mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP32 Actuators ^^");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // mqttpayload_room_temp [mqttpayloadSize] = {'\8'};
    // mqttpayload_hall_temp [mqttpayloadSize] = {'\8'};
    // mqttpayload_extern [mqttpayloadSize] = {'\0'};
    // mqttpayload_weather [mqttpayloadSize] = {'\0'};

    // Debuging Messages
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message:");

    if (strcmp(topic, "home/room/window/airquality") == 0){
      mqttpayload_extern = "";
      //Serial.println("Sono dentro extern\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqttpayload_extern.concat((char) payload[i]);
      }
    }

    if (strcmp(topic, "home/room/temperature")== 0){
      mqttpayload_room_temp = "";
      //Serial.println("Sono dentro room temp\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqttpayload_room_temp.concat((char) payload[i]);

      }
    }
    Serial.println();

    if (strcmp(topic, "home/hall/temperature")== 0){
      mqttpayload_hall_temp = "";
      //Serial.println("Sono dentro hall temp\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqttpayload_hall_temp.concat((char) payload[i]);
      }
    }

   Serial.println();

    if (strcmp(topic, "weather/")== 0){
      mqttpayload_weather = "";
      //Serial.println("Sono dentro weather\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqttpayload_weather.concat((char) payload[i]);
      }
    }

    Serial.println();

    if (strcmp(topic, "home/actuators")== 0){
      //Serial.println("Sono dentro home/actuators\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
      }
    }
    
    Serial.println();

    if (strcmp(topic, "home/room/window/humidity")== 0){
      mqttpayload_humidity_outside = "";
      //Serial.println("Sono dentro weather\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqttpayload_humidity_outside.concat((char) payload[i]);
      }
    }
    
    Serial.println();

    if (strcmp(topic, "home/room/window/temperature")== 0){
      mqttpayload_temperature_outside = "";
      //Serial.println("Sono dentro weather\n");
      for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqttpayload_temperature_outside.concat((char) payload[i]);
      }
    }    

    Serial.println();
    Serial.println("-----------------------");
}

// Song Parameters
int notes = 0;
int tempo = 0;
int wholenote = 0;
int divider = 0, noteDuration = 0;

void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }

  mqtt_client.loop();

  float home_temp = (mqttpayload_room_temp.toFloat() + mqttpayload_hall_temp.toFloat())/2;
  float outside_temp = mqttpayload_temperature_outside.toFloat();
  float outside_humidity =  mqttpayload_humidity_outside.toFloat();

  String outside_res = "";

  if (outside_temp < 31){
    if (outside_humidity < 75){
      outside_res = "good";
    }
  } else if (outside_humidity < 50){
    outside_res = "maybe";
  }
  
  // Debugging Print
  // Serial.println("Final values recorded (extern, room temp, hall tempo, weather forecast 1 day): \n");
  // Serial.println(mqttpayload_extern);
  // Serial.println();
  // Serial.println(mqttpayload_room_temp);
  // Serial.println();
  // Serial.println(mqttpayload_hall_temp);
  // Serial.println();
  // Serial.println(mqttpayload_weather);
  // Serial.println();
  // Serial.println("-----------------------");

  if (strstr(mqttpayload_weather.c_str(), "rain")){
      //Serial.println("Tomorrow is goin to rain: ");
      for (int i = 0; i < 3; i++){
        setColor(0,0,255);
        delay(500);
        setColor(255,255,255);
       }         
  }



  if (strstr(mqttpayload_extern.c_str(), "Good") != NULL || strstr(mqttpayload_extern.c_str(), "Moderate") != NULL) {
    if (strstr(outside_res.c_str(), "good") != NULL){
      Serial.println("outside temperature: " + mqttpayload_temperature_outside + "\n");
      Serial.println("outside h2O: " + mqttpayload_humidity_outside + "\n");
      if (home_temp > 24 || home_temp < 14){


      if (good == 0){
        good = 1;
        maybe = 0;
        bad = 0;

        setColor(0,255,0);
        mqtt_client.publish(mqtt_topic_actuators, "WINDOWS OPEN");

        tempo = 88;
        notes = sizeof(melody_good) / sizeof(melody_good[0]) / 2;
        wholenote = (60000 * 4) / tempo;
        divider = 0, noteDuration = 0;
        for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

            // calculates the duration of each note
            divider = melody_good[thisNote + 1];
            if (divider > 0) {
              // regular note, just proceed
              noteDuration = (wholenote) / divider;
            } else if (divider < 0) {
              // dotted notes are represented with negative durations!!
              noteDuration = (wholenote) / abs(divider);
              noteDuration *= 1.5; // increases the duration in half for dotted notes
            }

            // we only play the note for 90% of the duration, leaving 10% as a pause
            tone(BUZZER_PIN, melody_good[thisNote], noteDuration*0.9);

            // Wait for the specief duration before playing the next note.
            delay(noteDuration);
            
            // stop the waveform generation before the next note.
            noTone(BUZZER_PIN);
          }
        }
      }
    } else if (strstr(outside_res.c_str(), "maybe") != NULL){
      Serial.println("outside temperature: " + mqttpayload_temperature_outside + "\n");
      Serial.println("outside h2O: " + mqttpayload_humidity_outside + "\n");

        if (maybe == 0){
          maybe = 1;
          good = 0;
          bad = 0;
          setColor(255,255,0);
          mqtt_client.publish(mqtt_topic_actuators, "MAYBE WINDOWS OPEN");
          tempo = 105;
          notes = sizeof(melody_maybe) / sizeof(melody_maybe[0]) / 2;
          wholenote = (60000 * 4) / tempo;
          divider = 0, noteDuration = 0;
          for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

              // calculates the duration of each note
              divider = melody_maybe[thisNote + 1];
              if (divider > 0) {
                // regular note, just proceed
                noteDuration = (wholenote) / divider;
              } else if (divider < 0) {
                // dotted notes are represented with negative durations!!
                noteDuration = (wholenote) / abs(divider);
                noteDuration *= 1.5; // increases the duration in half for dotted notes
              }

              // we only play the note for 90% of the duration, leaving 10% as a pause
              tone(BUZZER_PIN, melody_maybe[thisNote], noteDuration * 0.9);

              // Wait for the specief duration before playing the next note.
              delay(noteDuration);

              // stop the waveform generation before the next note.
              noTone(BUZZER_PIN);
            }
        }
    }
  } else if (strstr(mqttpayload_extern.c_str(), "Unhealthy for Sensitive Groups") != NULL || strstr(mqttpayload_extern.c_str(), "Unhealthy") != NULL || strstr(mqttpayload_extern.c_str(), "Very Unhealthy") != NULL || strstr(mqttpayload_extern.c_str(), "Hazardous") != NULL) {
      if (home_temp < 33){
        if (bad == 0) {
          setColor(255,0,0);
          mqtt_client.publish(mqtt_topic_actuators, "WINDOWS CLOSED");
          good = 0;
          maybe = 0;
          bad = 1;

          tempo = 225;
          notes = sizeof(melody_bad) / sizeof(melody_bad[0]) / 2;
          wholenote = (60000 * 4) / tempo;
          divider = 0, noteDuration = 0;

          for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

              // calculates the duration of each note
              divider = pgm_read_word_near(melody_bad+thisNote + 1);
              if (divider > 0) {
                // regular note, just proceed
                noteDuration = (wholenote) / divider;
              } else if (divider < 0) {
                // dotted notes are represented with negative durations!!
                noteDuration = (wholenote) / abs(divider);
                noteDuration *= 1.5; // increases the duration in half for dotted notes
              }

              // we only play the note for 90% of the duration, leaving 10% as a pause
              tone(BUZZER_PIN, pgm_read_word_near(melody_bad+thisNote), noteDuration * 0.9);

              // Wait for the specief duration before playing the next note.
              delay(noteDuration);

              // stop the waveform generation before the next note.
              noTone(BUZZER_PIN);
          } 
      }
    } else {
        setColor(255,255,255);
        mqtt_client.publish(mqtt_topic_actuators, "DANGER");

        tempo = 88;
        notes = sizeof(melody_danger) / sizeof(melody_danger[0]) / 2;
        wholenote = (60000 * 4) / tempo;
        divider = 0, noteDuration = 0;
        for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

          // calculates the duration of each note
          divider = melody_danger[thisNote + 1];
          if (divider > 0) {
            // regular note, just proceed
            noteDuration = (wholenote) / divider;
          } else if (divider < 0) {
            // dotted notes are represented with negative durations!!
            noteDuration = (wholenote) / abs(divider);
            noteDuration *= 1.5; // increases the duration in half for dotted notes
          }

          // we only play the note for 90% of the duration, leaving 10% as a pause
          tone(BUZZER_PIN, melody_danger[thisNote], noteDuration*0.9);

          // Wait for the specief duration before playing the next note.
          delay(noteDuration);
            
          // stop the waveform generation before the next note.
          noTone(BUZZER_PIN);
        }
    }
  } else {
    setColor(0,0,0);
  }
  delay(100);
}

void setColor(int R, int G, int B) {
  analogWrite(PIN_RED,   R);
  analogWrite(PIN_GREEN, G);
  analogWrite(PIN_BLUE,  B);
}