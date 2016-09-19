/* Firmware for the Smart Shelf

   This is the ESP8266 Arduino firmware (NodeMCU ESP-12E) for the Smart Shelf electronics described at
   https://tinker.yeoman.com.au/2016/01/15/esp8266-smart-shelf-part-2-nodemcu-arduino-ide/

   The latest version can be found at
   https://github.com/tinkermax/smartshelf

   Malcolm Yeoman (2016)

   Note: Wi-Fi username and password and MQTT Broker
         need to be specified in Config.h file
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Config.h"

#define DEBUG false
//Comment out the following line to allow debugging info on serial monitor
//#define Serial if(DEBUG)Serial

//Wi-Fi username and password
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;

//Specify the address of MQTT Broker in the Config.h file
const char* mqtt_server = MQTTBROKER;

//Set Sensor ID
#define SensorID "112334556" //Change this to your number!
#define mqttPubTopic "/Sshelf/" SensorID
#define randName "SShelf" SensorID __DATE__ __TIME__

//Ultrasonic Trigger pin is on GPIO12 (D6)
const byte trigPin = 12;
//Ultrasonic Echo pin is on GPIO13 (D7)
const byte echoPin = 13;

long last_distance = 0;
long last_reading = 0;
long second_last_reading = 0;

WiFiClient wclient;
PubSubClient client(wclient);

void setup() {
  // Setup console
  Serial.begin(115200);
  Serial.println("Booting");

  //Initialise input/output pins
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Switch on LED to indicate initialisation
  digitalWrite(BUILTIN_LED, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    Serial.print(".");
    delay(30000);
    ESP.restart();
  }

  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //Initialise MQTT
  client.setServer(mqtt_server, 1883);

  //Switch off LED
  digitalWrite(BUILTIN_LED, 1);
}

void loop() {
  long duration, distance;
  char buffer[20];

  //client.loop();

  if (WiFi.status() == WL_CONNECTED) {

    //Confirm that still connected to MQTT broker
    if (!client.connected()) {
      Serial.println("Reconnecting to MQTT Broker");
      reconnect();
    }
  }

  //Switch on LED
  digitalWrite(BUILTIN_LED, 0);

  // send the pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2); // low for 2 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); // high for 10 microseconds
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH); // measure the time to the echo
  distance = (duration / 2) / 2.91; // calculate the distance in mm

  //Limit Detected Range to between 10mm and 400mm
  if (distance < 10) {
    distance = 10;
  }

  if (distance > 400) {
    distance = 400;
  }

  //Noise control: if deviation of last 3 readings not consistent (e.g. movement), ignore
  if (abs(distance - last_reading) > 5 || abs(last_reading - second_last_reading) > 5) {
    second_last_reading = last_reading;
    last_reading = distance;
    distance = last_distance;
  }
  else {
    second_last_reading = last_reading;
    last_reading = distance;
  }

  //Only do something if measured distance has changed by >= 10mm since last time
    if (abs(distance - last_distance) >= 10) {
      last_distance = distance;
      sprintf(buffer, "%d", distance);
      client.publish(mqttPubTopic, buffer);
      Serial.println(buffer);
    }
    else {
      distance = last_distance;
    }

  //Switch off LED
  digitalWrite(BUILTIN_LED, 1);

  client.loop();                                
  delay(300);
}

void reconnect() {
  // Loop until we're reconnected to MQTT Broker
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection as ");
    Serial.print(randName);
    Serial.print("...");
    // Attempt to connect
    if (client.connect(randName)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}
