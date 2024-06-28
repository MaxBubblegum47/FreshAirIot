#include "pitches.h"
#include "WifiPassword.h"
#include "mqttCredentials.h"
#include <PubSubClient.h>
#include <WiFi.h>

#define BUZZER_PIN  5

#define PIN_RED    19 // GPIO23
#define PIN_GREEN  23 // GPIO22
#define PIN_BLUE   18 // GPIO21

int melody[] = {
  
  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,

  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,
  

  NOTE_E5,2,  NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,2,   NOTE_A4,2,
  NOTE_GS4,2,  NOTE_B4,4,  REST,8, 
  NOTE_E5,2,   NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,4,   NOTE_E5,4,  NOTE_A5,2,
  NOTE_GS5,2,
};


const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint

const char *mqtt_topic_temperature_room = "home/room/temperature";
const char *mqtt_topic_temperature_hall = "home/hall/temperature";
const char *mqtt_topic_actuators = "home/actuators";

const char *mqtt_username = username;  // MQTT username for authentication
const char *mqtt_password = password_mqtt;  // MQTT password for authentication
const int mqtt_port = 1883;  // MQTT port (TCP)
const int mqttpayloadSize = 100;
char mqttpayload_room_temp [mqttpayloadSize] = {'\0'};
char mqttpayload_hall_temp [mqttpayloadSize] = {'\0'};
char mqttpayload_actuators [mqttpayloadSize] = {'\0'};
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

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
        String client_id = "esp32 actuators" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic_temperature_room);
            mqtt_client.subscribe(mqtt_topic_temperature_hall);
            mqtt_client.subscribe(mqtt_topic_actuators);            
            
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
    mqttpayload_room_temp [mqttpayloadSize] = {'\0'};
    mqttpayload_hall_temp [mqttpayloadSize] = {'\0'};
    mqttpayload_actuators [mqttpayloadSize] = {'\0'};

    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message:");

    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    
    Serial.println();

    if (strcmp(topic, "home/actuators") == 0){
          memcpy(mqttpayload_actuators, payload, length);
          Serial.println("Sono dentro il check di 1");
    }

    if (!strcmp(topic, "home/room/temperature") == 0){
          memcpy(mqttpayload_room_temp, payload, length);
          Serial.println("Sono dentro il check di 2");

    }

    if (!strcmp(topic, "home/hall/temperature") == 0){
          memcpy(mqttpayload_hall_temp, payload, length);
          Serial.println("Sono dentro il check di 3");

    }
    Serial.println();
    Serial.println("-----------------------");
}

int notes = sizeof(melody) / sizeof(melody[0]) / 2;

int tempo = 144;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;

void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }

  mqtt_client.loop();

  // color code #00C9CC (R = 0,   G = 201, B = 204)
  setColor(255, 0, 0);

  char *str2 = "OPEN";
  int value = strcmp(mqttpayload_actuators,str2);

  if (value == 0){
    setColor(0,255,0);
  }
  delay(1000);

  //delay(3000); // keep the color 1 second

  // color code #F7788A (R = 247, G = 120, B = 138)
  //setColor(0, 255, 0);

  //delay(3000); // keep the color 1 second

  // color code #34A853 (R = 52,  G = 168, B = 83)
  //setColor(0, 0, 255);

  //delay(3000); // keep the color 1 second

  // song
  // for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

  //   // calculates the duration of each note
  //   divider = melody[thisNote + 1];
  //   if (divider > 0) {
  //     // regular note, just proceed
  //     noteDuration = (wholenote) / divider;
  //   } else if (divider < 0) {
  //     // dotted notes are represented with negative durations!!
  //     noteDuration = (wholenote) / abs(divider);
  //     noteDuration *= 1.5; // increases the duration in half for dotted notes
  //   }

  //   // we only play the note for 90% of the duration, leaving 10% as a pause
  //   tone(BUZZER_PIN, melody[thisNote], noteDuration*0.9);

  //   // Wait for the specief duration before playing the next note.
  //   delay(noteDuration);
    
  //   // stop the waveform generation before the next note.
  //   noTone(BUZZER_PIN);
  // }


  // for (int thisNote = 0; thisNote < sizeof(melody); thisNote++) {
  //   int noteDuration = 1000; // noteDurations[thisNote];
  //   tone(BUZZER_PIN, melody[thisNote], noteDuration);

  //   int pauseBetweenNotes = noteDuration * 1.30;
  //   delay(pauseBetweenNotes);
  //   noTone(BUZZER_PIN);
  // }
}

void setColor(int R, int G, int B) {
  analogWrite(PIN_RED,   R);
  analogWrite(PIN_GREEN, G);
  analogWrite(PIN_BLUE,  B);
}