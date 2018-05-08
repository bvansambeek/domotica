// This basic sketch can:
// - connect and reconnect to wifi (non_blocking)
// - connect to MQTT server
// - OTA support
// - RSSI info of ESP8266
// - internal LED on means something is not right, off is everything is connected
// - ESP8266 board version 2.4.1 required (not 2.4.0 - it will not reconnect)

// Home assistant configuration.yaml example of RSSI sensor:
// - platform: mqtt
//   state_topic: 'home/midden/keuken/spots/RSSI'
//   name: 'keuken RSSI'
//   unit_of_measurement: 'dB'

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;  //makes sure that the wifi stays connected
float rssi;                  // To measure signal strenght
int rssiinterval = 60000;    // ms interval between checkups (reality is longer as rest of code takes more time).
long lastrssi = 0;           // counter for non-interupt measurement

// Update these with values suitable for your network.
const char* ssid  = "........";
const char* password = "........r";
const char* mqtt_server = "192.168.178.123";
int mqttport = 1883;
#define CLIENT_ID   "keukenspots"                      //has to be unique in your system
#define MQTT_TOPIC  "home/midden/keuken/spots"         //has to be identical with home assistant configuration.yaml

WiFiClient espClient;
PubSubClient client(espClient);
long lastReconnectAttempt = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(LED_BUILTIN, LOW);  // LED on

  //initialize wifi event handlers
  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    Serial.print("Station connected - IP: ");
    Serial.println(WiFi.localIP());
  });

  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    Serial.println("Station disconnected");
    digitalWrite(LED_BUILTIN, LOW);  // LED on
  });

  Serial.printf("Connecting to %s ...\n", ssid);
  WiFi.begin(ssid, password);
  
  client.setServer(mqtt_server, mqttport);
  client.setCallback(callback);
  lastReconnectAttempt = 0;

 // delay(4000); // delay to make sure wifi is connected before ota setup

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
  
}

void loop() {
  ArduinoOTA.handle();     //OTA support
  reconnectMQTT();         //reconnect if MQTT is not connected (wifi reconnect is handled by ESP chip event handler
  checkrssi();             // Check RSSI and send update over MQTT

  // do your loop business here

  
}

void reconnectMQTT() {
    if (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);  // LED on
    long now2 = millis();
    if (now2 - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now2;
      // Attempt to reconnect MQTT
      Serial.println("Attempting MQTT connection...");
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Do something with received message
  if ((char)payload[0] == '1') {
    digitalWrite(LED_BUILTIN, LOW);
    client.publish(MQTT_TOPIC, "1", true);
   // lightstate = 1;
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    client.publish(MQTT_TOPIC, "0", true);
   // lightstate = 0;
  }
}

boolean reconnect() {
  if (client.connect(CLIENT_ID)) {
    Serial.println("connected");
    digitalWrite(LED_BUILTIN, HIGH);  // LED off
    //publish lightstates etc.
    //client.publish("outTopic","hello world");
    // ... and resubscribe
    client.subscribe(MQTT_TOPIC"/set"); 
  }
  return client.connected();
}
