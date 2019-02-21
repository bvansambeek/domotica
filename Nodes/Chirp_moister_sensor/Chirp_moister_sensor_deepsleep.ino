// Connect VCC to 3v and SDA (D2), SCL (D1) and ground to D8 Wemos
// Made by bvansambeek, 2019-02-18 hope this helps someone.
// Most of the code comes from the site of the sensor:
// https://wemakethings.net/chirp/
// and this example
// https://gist.github.com/Miceuz/8ace1cde27671e8e161d

#include <Wire.h>
#define chirpVCC D5                   // connected to VCC of Chirp
#define chirpGND D8                   // connected to GND of Chirp
#define chirpreset D3                 // connected to RESET of Chirp
int moisture = 21000;                 

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("setup");  
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);                // Wemos buildin_LED off (for better WAF)
  
  pinMode(D1, INPUT_PULLUP);                      // setting D1 and D2 to pullup for better I2C communication
  pinMode(D2, INPUT_PULLUP);                      // setting D1 and D2 to pullup for better I2C communication
  pinMode(chirpGND, OUTPUT);                      // (option for disabling it during sleep)
  pinMode(chirpVCC, OUTPUT);                      // (option for disabling it during sleep)
  pinMode(chirpreset, OUTPUT);                    // To reset Chirp until it gives a normal reading
  digitalWrite(chirpreset, HIGH);                 // Start with chirp reset off (Change HIGH to LOW for non wemos application)

  digitalWrite(chirpVCC, HIGH);                   // Turn on Chirp
  digitalWrite(chirpGND, LOW);                    // Turn on Chirp
  delay(100);                                     // Allow some time for chirp to boot
  writeI2CRegister8bit(0x20, 3);                  // Send something on the I2C bus to put chirp into sensor mode
  delay(1000);                                    // allow chirp to boot

  // Put Chirp into sensor mode by resetting in a while loop if the sensor data does not make any sense
  while (moisture < 0 || 20000 < moisture){      // while loop for setting chirp to slave mode
    Serial.println("in while loop");             // print to serial monitor to check
    digitalWrite(chirpreset, LOW);               // reset chirp on
    delay(200);                                  // wait a little bit; 200ms for pressing button?
    digitalWrite(chirpreset, HIGH);              // reset chirp off
    delay(500);                                  // wait a little bit
    moisture = readI2CRegister16bit(0x20, 0);    // read capacitance register (send I2C communication)
    Serial.print("Moisture (from while loop) = ");
    Serial.print(moisture);
    delay(500);
  }
  
  moisture = readI2CRegister16bit(0x20, 0);       // Read moisture and send to serial monitor
  Serial.print("Moisture = ");
  Serial.println(moisture);  
  digitalWrite(chirpGND, HIGH);                   // Turn off Chirp 
  digitalWrite(chirpVCC, LOW);                    // Turn off Chirp        

  Serial.println("going to sleep biep biep");
  int sleepseconds = 5;
  ESP.deepSleep(sleepseconds * 1000000);
//  const float sleephours = 1;
//  ESP.deepSleep(sleephours * 1000000 * 60 * 60);
}

void loop() {    
  // This code is made for ESP8266 Deep Sleep and is therefore not using loop
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
