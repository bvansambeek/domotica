// Made by bvansambeek, shared under MIT licence
// Hope you have as much fun as I have when your project succeeds! ;)
//
// Wemos D1 mini with:
//- DS18B20 digital temperature sensor - GND - DATA (D7) - 5v - Put 4.7k resistor between 5v and data
//- 3 relays
//- 4 momentary buttons 
//- OTA support
//- RSSI sensor
//
//Home assistant configuration:
//
//sensor:
// - platform: mqtt
//   state_topic: 'home/midden/woonkamer/btn/temp'
//   name: 'woonkamer temperatuur'
//   unit_of_measurement: '°C'
//
// - platform: mqtt
//   state_topic: 'home/midden/woonkamer/btn/rssi'
//   name: 'woonkamer plafon RSSI'
//   unit_of_measurement: "dBm"
//
//switch:
// - platform: mqtt
//   name: "woonkamer Relay 1"
//   state_topic: "home/midden/woonkamer/btn/relay1"
//   command_topic: "home/midden/woonkamer/btn/relay1/set"
//   payload_off: "0"
//   payload_on: "1"
//   qos: 1
//
// - platform: mqtt
//   name: "woonkamer Relay 2"
//   state_topic: "home/midden/woonkamer/btn/relay2"
//   command_topic: "home/midden/woonkamer/btn/relay2/set"
//   payload_off: "0"
//   payload_on: "1"
//   qos: 1
//
// - platform: mqtt
//   name: "woonkamer Relay 3"
//   state_topic: "home/midden/woonkamer/btn/relay3"
//   command_topic: "home/midden/woonkamer/btn/relay3/set"
//   payload_off: "0"
//   payload_on: "1"
//   qos: 1
//
//automation:
//- alias: btn press 1
//  trigger:
//    platform: mqtt
//    topic: home/midden/woonkamer/btn/btn1
//    payload: '1'
//  action:
//  #do something

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

float rssi;                  // To measure signal strenght
int rssiinterval = 10000;     // ms interval between checkups (reality is longer as rest of code takes more time).
long lastrssi = 0;           // counter for non-interupt measurement

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D7       // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int checkinterval = 15000;   // ms interval between checkups (reality is longer as rest of code takes more time), smaller then 6 seconds makes it unstable.
long lastMsg = 0;            // counter for non-interupt measurement

      //4 momentary buttons
      #define button1 D1
      int button1State = 0;
      #define button2 D2
      int button2State = 0;
      #define button3 D3
      int button3State = 0;
      #define button4 RX
      int button4State = 0;

// Update these with values suitable for your network.
const char* ssid  = "SUPER FLY";
const char* password = "FOR WIFI";
const char* mqtt_server = "192.168.0.1";
int mqttport = 1883;
#define CLIENT_ID   "woonkamer.knop"                    //has to be unique in your system
#define MQTT_TOPIC  "home/midden/woonkamer/btn"         //has to be identical with home assistant configuration.yaml

WiFiClient espClient;
PubSubClient client(espClient);
      
//RGB LED
#define relay1 D0
#define relay2 D5
#define relay3 D6
const char* topic_relay1 = MQTT_TOPIC"/relay1/set";
const char* topic_relay2 = MQTT_TOPIC"/relay2/set";
const char* topic_relay3 = MQTT_TOPIC"/relay3/set";
int light = 0;
 
void setup(){
  Serial.begin(9600);
  Serial.println("setup begin");
  pinMode(LED_BUILTIN, OUTPUT);
  
  DS18B20.begin();
       
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  delay(10);
  digitalWrite(relay1, HIGH);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(relay2, HIGH);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(relay3, HIGH);           //start with off (I used SSR, if you use regular then you should use LOW)
  digitalWrite(LED_BUILTIN, HIGH);

       // test relays for 1 second
       digitalWrite(LED_BUILTIN, LOW);
       delay(1000);
       digitalWrite(LED_BUILTIN, HIGH);
       digitalWrite(relay1, LOW);
       delay(1000);
       digitalWrite(relay1, HIGH);
       digitalWrite(relay2, LOW);
       delay(1000);
       digitalWrite(relay2, HIGH);
       digitalWrite(relay3, LOW);
       delay(1000);
       digitalWrite(relay3, HIGH);

       pinMode(button1, INPUT_PULLUP);
       pinMode(button2, INPUT_PULLUP);
       pinMode(button3, INPUT_PULLUP);
       pinMode(button4, INPUT_PULLUP);

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
  ArduinoOTA.handle();

  // Connect to Wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
    reconnect();
    client.setServer(mqtt_server, mqttport);
    client.setCallback(callback);
  } else {
    //if connected get RSSI (Wifi signal strengt) from ESP8266 and publishes it to MQTT
    checkrssi();
  }

  // reconnect when MQTT is not connected
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // temperature
  checktemp();

  //listen to buttons
        int debugserial = 0;    // make 1 if you want to see the button status in serial monitor
        button1State = digitalRead(button1); 
        if (button1State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn1", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH);
        }
        if (debugserial == 1){Serial.print("1 = ");Serial.print(button1State);}

         // read button press
        button2State = digitalRead(button2); 
        if (button2State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn2", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
        } 
        if (debugserial == 1){Serial.print(" - 2 = ");Serial.print(button2State);}

         // read button press
        button3State = digitalRead(button3); 
        if (button3State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn3", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
        }
        if (debugserial == 1){Serial.print(" - 3 = ");Serial.print(button3State);}

        // read button press
        button4State = digitalRead(button4); 
        if (button4State == LOW) {
          digitalWrite(LED_BUILTIN, LOW); 
          client.publish(MQTT_TOPIC"/btn4", "1", true);
          delay(200); // delay to prevent double click
          digitalWrite(LED_BUILTIN, HIGH); 
        }   
        if (debugserial == 1){Serial.print(" - 4 = ");Serial.println(button4State);}
  
}

void checktemp(){
  long now = millis();                  // only check every x ms  checkinterval
  if (now - lastMsg > checkinterval) {
    lastMsg = now;

        DS18B20.requestTemperatures(); 
        String temps1 = String(DS18B20.getTempCByIndex(0));
//        String temps2 = String(DS18B20.getTempCByIndex(1));   //you are able to connect up to 100-1000 of these bad boys on that one sigle port
        float t = temps1.toFloat();
//        float t2 = temps2.toFloat();
        Serial.print("Temp1: ");
        Serial.println(t);

    if (client.connected()) {
      client.publish(MQTT_TOPIC"/temp", String(t).c_str(), false);     
    }
  }
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
        delay(10);
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
  digitalWrite(LED_BUILTIN, HIGH);           //LED off
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //switch on relay1 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay1)==0){      
    if ((char)payload[0] == '1') {
      digitalWrite(relay1, LOW);                                  //on  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay1", "1", true);
    } else {
      digitalWrite(relay1, HIGH);                                 //off  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay1", "0", true);
    }
  }

  //switch on relay2 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay2)==0){
    if ((char)payload[0] == '1') {
      digitalWrite(relay2, LOW);                                  //on  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay2", "1", true);
    } else {
      digitalWrite(relay2, HIGH);                                 //off  (you might need to change this for regular relays, I used SSR)
      client.publish(MQTT_TOPIC"/relay2", "0", true);
    }
  }

  //switch on relay3 LED if topic is correct and 1 is received
  if (strcmp(topic, topic_relay3)==0){
    if ((char)payload[0] == '1') {
      client.publish(MQTT_TOPIC"/relay3", "1", true);
      digitalWrite(relay3, LOW);                                  //on  (you might need to change this for regular relays, I used SSR)
    } else {
      client.publish(MQTT_TOPIC"/relay3", "0", true);
      digitalWrite(relay3, HIGH);                                 //off  (you might need to change this for regular relays, I used SSR)
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  digitalWrite(LED_BUILTIN, LOW);     //LED on
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC"/relay1/set");    //subscribe relay to MQTT server, you can set your name here
      client.subscribe(MQTT_TOPIC"/relay2/set");    //subscribe relay to MQTT server, you can set your name here
      client.subscribe(MQTT_TOPIC"/relay3/set");    //subscribe relay to MQTT server, you can set your name here
      digitalWrite(LED_BUILTIN, HIGH);           //LED off
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait (500 * 10ms = 5000ms) 5 seconds before retrying, but read button press meanwhile
      for (int i=0; i <= 500; i++){
        delay(10);
        if (i == 50) { 
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

