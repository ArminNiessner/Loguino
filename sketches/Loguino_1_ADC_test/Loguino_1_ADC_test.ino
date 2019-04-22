//Reads the value from an AD converter MCP342x and displays it in the serial monitor




#include <Wire.h>
#include <MCP342x.h>

const byte addr = 4;      // ADC: I2C address - 0:68, 1:69, 2:6A, 3:6B, 4:6C, 5:6D, 6:6E, 7:6F
const uint8_t gain = 0;   // ADC: gain factor 0-3: x1, x2, x4, x8
const uint8_t rate = 3;   // ADC: resolution 0-3: 12bit ... 18bit
const uint8_t mode = 0;   // ADC: mode 0 == one-shot mode - 1 == continuos mode
//  create an objcet of the class MCP342x
MCP342x mcp342x = MCP342x();

const uint8_t enable = 8;

float span = 0.0;
char str_buf[50] = "                            ";

void setup()
{
  Serial.begin(9600);
  pinMode(enable, OUTPUT);
  Serial.print("General Call Reset... ");
  Wire.beginTransmission(B00000000);
  Wire.write(B00000110);
  Wire.endTransmission();
  Serial.println("fertig");
  
}



void loop()
{
 
  digitalWrite(enable, HIGH);
  for(uint8_t ch=0; ch<4;ch++){
    if(ch)
      Serial.print("\t");
      Serial.print("C ");
      Serial.print(ch);
      mcp342x.setConf(addr, 1, ch, mode, rate, gain);
      delay(300);
      span = mcp342x.getData(addr);
  
      //  convert float to char[]
      dtostrf(span, 10, 6, str_buf);
  
      Serial.print(str_buf);
  }
  Serial.println();

  digitalWrite(enable, LOW); 

   
  //  do it every n milliseconds
  delay(2000);
}
