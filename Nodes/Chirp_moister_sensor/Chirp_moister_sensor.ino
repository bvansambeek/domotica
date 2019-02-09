// Connect VCC to 3v and SDA, SCL and ground to Wemos
// Also connect D3 to the reset of the Chrip sensor
// Made by bvansambeek, 2019-02-09 hope this helps someone.
// Most of the code comes from the site of the sensor:
// https://wemakethings.net/chirp/

#include <Wire.h>

#define chirpreset D3                 // connected to RESET of Chirp

int moisture = 21000;                 // start value that does not make any sense for moisture to go into while loop
int light;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("setup");
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  pinMode(chirpreset, OUTPUT); 
  digitalWrite(chirpreset, HIGH);
}

void loop() {  
  
  // Put Chirp into sensor mode by resetting in a while loop if the sensor data does not make any sense
  while (moisture < 0 || 20000 < moisture){      // while loop for setting chirp to slave mode
    Serial.println("in while loop");             // print to serial monitor to check
    digitalWrite(BUILTIN_LED, LOW);              // LED on to show it is putting chrip to slave by resetting
    digitalWrite(chirpreset, LOW);               // reset chirp on
    delay(200);                                  // wait a little bit; 200ms for pressing button?
    digitalWrite(chirpreset, HIGH);              // reset chirp off
    delay(500);                                  // wait a little bit
    digitalWrite(BUILTIN_LED, HIGH);             // LED off to show it is putting chrip to slave
    moisture = readI2CRegister16bit(0x20, 0);    // read capacitance register (send I2C communication)
    Serial.println(moisture);
    delay(500);
  } 
  
  // make sure the buildin LED is off and 
  if (0 <= moisture && moisture <= 20000) {
    digitalWrite(BUILTIN_LED, HIGH);             // LED off, because a valid moisture value was recorded
    digitalWrite(chirpreset, HIGH);              // reset off
  }

  // Read moisture and send to serial monitor
  moisture = readI2CRegister16bit(0x20, 0);      // read capacitance register
  Serial.print("Moisture = ");
  Serial.println(moisture);               
  delay(500);

  // Read light and send to serial monitor (optional, you can remove this part if you do not care about the light)
  writeI2CRegister8bit(0x20, 3);          // request light measurement 
  delay(9000);                            // this can take a while
  light = readI2CRegister16bit(0x20, 4);  // ready light register
  Serial.print("Light = ");
  Serial.println(light);

  delay(2000);                            // to prevent the loop going crazy
}

void writeI2CRegister8bit(int addr, int value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

unsigned int readI2CRegister16bit(int addr, int reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  delay(1100);
  Wire.requestFrom(addr, 2);
  unsigned int t = Wire.read() << 8;
  t = t | Wire.read();
  return t;
}
