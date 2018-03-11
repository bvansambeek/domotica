# domotica
A gathering of code for nodes with different hardware connected to HASS (home assistant)

// Made by bvansambeek, shared under MIT licence
// Hope you have as much fun as I have when your project succeeds! ;)

Nodes folder:

Wemos_MQQT_4btns_3relays_temp (code works on any ESP8266 based device, you only need to change ports)
- DS18B20 digital temperature sensor (multiple on 1 port optional)
- 3 relays (or Relays)
- 4 momentary buttons 
- OTA support
- RSSI sensor


Sonoff S20 (code works on any ESP8266 based device, you only need to change ports)
Including:
- reconnect when MQTT or WIFI is lost and still listen to button.
- OTA support
- RSSI sensor
- ability to controll light without connection of wifi or MQQT server through button
- Very high WAF (https://en.wikipedia.org/wiki/Wife_acceptance_factor)
