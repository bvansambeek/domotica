// Made by bvansambeek, shared under MIT licence
// Hope you have as much fun as I have when your project succeeds! ;)
//
//Sonoff s20 with:
//- relay - connected to blue led
//- Green LED
//- Momentary button
//- OTA support
//- RSSI sensor
//- Extra High
//
//Home assistant configuration:
//
//switch:
// - platform: mqtt
//   name: "Woonkamer hoeklamp"
//   state_topic: "home/midden/woonkamer/hoeklamp1"
//   command_topic: "home/midden/woonkamer/hoeklamp1/set"
//   payload_off: "0"
//   payload_on: "1"
//   qos: 1
//
//sensor:
// - platform: mqtt
//   state_topic: 'home/midden/woonkamer/hoeklamp1/rssi'
//   name: 'keuken RSSI'
//   unit_of_measurement: "dBm"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
float rssi;                  // To measure signal strenght
int rssiinterval = 15000;     // ms interval between checkups (reality is longer as rest of code takes more time).
long lastrssi = 0;           // counter for non-interupt measurement

#define LED 13
#define RELAY 12       //including blue led
#define buttonPin 0
int buttonState = 0;
int lightstate;

// Update these with values suitable for your network.
const char* ssid      = "SUPER FLY";
const char* password  = "FOR WIFI";
const char* mqtt_server = "192.168.0.1";
int mqttport            = 1883;
#define CLIENT_ID         "woonkamer.hoeklamp1"                      //has to be unique in your system
#define MQTT_TOPIC        "home/midden/woonkamer/hoeklamp1"          //has to be identical with home assistant configuration.yaml

WiFiClient espClient;
PubSubClient client(espClient);
 
void setup(){
  Serial.begin(9600);
  Serial.println("setup begin");
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(RELAY, LOW);            // Start with lights off
  lightstate = 0;

  //start wifi and mqqt
  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // OTA setup
   ArduinoOTA.setHostname(CLIENT_ID);
   ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  Serial.println("setup end");
}

void loop(){
  Serial.println("Loop begin");
  ArduinoOTA.handle();

  // Connect to Wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("wifi connect in loop - start");
    setup_wifi();
    Serial.println("wifi connect in loop - wifi connected");
    reconnect();
    client.setServer(mqtt_server, mqttport);
    client.setCallback(callback);
    Serial.println("wifi connect in loop - wifi and MQTT connected");
  } else {
    Serial.println("wifi connect in loop - already connected");
    //getting RSSI (Wifi signal strengt) from ESP8266 and publishes it to MQTT
    checkrssi();
  }

  // reconnect when MQTT is not connected
  if (!client.connected()) {
    Serial.println("MQTT reconnect - start");
    reconnect();

    //publish state here (in case wifi is not connected then lightstate might changed before connecting MQTT)
    if (lightstate == 1){
      client.publish(MQTT_TOPIC, "1", true);
    } else {
      client.publish(MQTT_TOPIC, "0", true);
    }
    
    Serial.println("MQTT reconnect - end");
  } else {
    Serial.println("MQTT reconnect - already connected");
  }
  client.loop();

    // read button press
        buttonState = digitalRead(buttonPin); 
        if (buttonState == 0) {
          if (lightstate == 0){
            lightstate = 1;
            digitalWrite(RELAY, HIGH);   //Light on
            if (client.connected()) {client.publish(MQTT_TOPIC, "1", true);}
          } else {
            lightstate = 0;
            digitalWrite(RELAY, LOW);  //Light off
            if (client.connected()) {client.publish(MQTT_TOPIC, "0", true);}
          }
          delay(150); // delay after button press to prevent double click
        }
        Serial.print("lightstate = ");
        Serial.print(lightstate);
        Serial.print(" - buttonState = ");
        Serial.println(buttonState);   

  Serial.println("Loop ended");
}

void checkbtnpress(){
  buttonState = digitalRead(buttonPin);
  if (buttonState == 0) {
          if (lightstate == 0){
            lightstate = 1;
            digitalWrite(RELAY, HIGH);  //Light on
          } else {
            lightstate = 0;
            digitalWrite(RELAY, LOW);  //Light off
          }
          delay(150);
        } 
   delay(10);    
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  WiFi.mode(WIFI_STA); //ESP in connect to network mode
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    // wait 50 * 10 ms = 500 ms to give ESP some time to connect (need to give delay else you get soft WDT reset)
      for (int i=0; i <= 50; i++){
      checkbtnpress(); // this takes ~10ms
        if (i == 50) { 
          Serial.println(".");
        } else {
          Serial.print(".");
        }     
      }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the relay if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(RELAY, HIGH);
    client.publish(MQTT_TOPIC, "1", true);
    lightstate = 1;
    Serial.print("lightstate = ");
    Serial.println(lightstate);
  } else {
    digitalWrite(RELAY, LOW);
    client.publish(MQTT_TOPIC, "0", true);
    lightstate = 0;
    Serial.print("lightstate = ");
    Serial.println(lightstate);
  }
}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite(LED, LOW);           //LED on
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC"/set");    //subscribe light to MQTT server, you can set your name here
      digitalWrite(LED, HIGH);       //LED off
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait (500 * 10ms = 5000ms) 5 seconds before retrying, but read button press meanwhile
      for (int i=0; i <= 500; i++){
      checkbtnpress(); // this takes ~10ms
        if (i == 500) { 
          Serial.println(".");
        } else {
          Serial.print(".");
        }     
      }     
    }
  }
}

void checkrssi(){
  long nowrssi = millis();                  // only check every x ms  checkinterval
  if (nowrssi - lastrssi > rssiinterval) {
    lastrssi = nowrssi;
    // measure RSSI
    float rssi = WiFi.RSSI();

    // make RSSI -100 if long rssi is not between -100 and 0
    if (rssi < -100 && rssi > 0){
      rssi = -110;
    }
    
    Serial.print("RSSI = ");
    Serial.println(rssi);

    if (client.connected()) {
      client.publish(MQTT_TOPIC"/rssi", String(rssi).c_str(), false);   
    }
  }
}

