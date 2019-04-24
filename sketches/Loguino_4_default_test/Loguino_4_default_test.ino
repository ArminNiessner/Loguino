/*
 * Reads a potentiometer connected to ch1 and ch2, thermocouple connected to ch3, a DS18S20 Temperature sensor and displays it in the serial monitor
 * 
 * Copyright (C) 2019  Armin Niessner
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
 

#include <Wire.h>                       // lib for I2C (needed for ADC)
#include <SPI.h>                        // lib for SPI (needed for SD-Card module
#include <SD.h>                         // lib for SD-Card
#include <ANiess.h>               // needed for RTC
#include <AN_RTClib.h>                     // lib for Real-Time-Clock
#include <LowPower.h>                   // lib for sleep mode/low power mode
#include <AVRUtils.h>                   // needed to messure supply voltage
#include <DallasTemperature.h>          // lib for DS18S20 Temperature sensor
#include <MCP342x.h>                    // lib for ADC

#define ONE_WIRE_BUS 4                  // data pin for DS18S20
OneWire oneWire(ONE_WIRE_BUS);          
DallasTemperature sensors(&oneWire);   

AVRUtils avr;
DS3231 RTC;

File dataFile;
unsigned long Filesize;

const int chipSelect = 10;              // pin chipselect for SD-CardADC
const int enable_rtc = 5;               // pin to enable power supply to RTC
const int enable_temp = 6;              // pin to enable power supply to DS18S20
const int enable_dd1 = 8;               // pin to enable power supply to Dendrometer #1
//const int enable_dd2 = 9;             // pin to enable power supply to Dendrometer #2

uint8_t Log_Hour;                       // variable for hour to log
uint8_t Log_Minute;                     // variable for minute to log
uint8_t st_Hour;                        // variable for hour at logger start
uint8_t st_Minute;                      // variable for minute at logger start
uint8_t Log_Interval_Minutes = 1;       // logging intervall in minutes

int wakePin = 2;                 // pin used for waking up

const byte addr = 4;                    // ADC: I2C address - 0:68, 1:69, 2:6A, 3:6B, 4:6C, 5:6D, 6:6E, 7:6F
const uint8_t gain = 0;                 // ADC: gain factor 0-3: x1, x2, x4, x8
const uint8_t rate = 3;                 // ADC: resolution 0-3: 12bit ... 18bit
const uint8_t mode = 0;                 // ADC: mode 0 == one-shot mode - 1 == continuos mode

MCP342x mcp342x = MCP342x();            //  create an objcet of the class MCP342x

int i_file = 0;                         // initial file appendix (a new file will be created for every 1MB file size)
String filename;                        
float vdd1;                             // voltage dd1
float dd1;                              // cm dd1
char str_dd1[10] = "";
//float vdd2;                             // voltage dd2
//float dd2;                              // cm dd2
//char str_dd2[10] = "";
float batV;                             // Battery voltage, supply voltage
char str_batV[10] = "";
float xyl1;                             // xylem sensor #1
char str_xyl1[10] = "";
//float xyl2;                             // xylem sensor #2
//char str_xyl2[10] = "";

float temp;



void setup() {
  pinMode(enable_rtc, OUTPUT);          // enable RTC
  digitalWrite(enable_rtc, HIGH);
  pinMode(wakePin, INPUT_PULLUP);
  pinMode(enable_temp, OUTPUT);  
  pinMode(enable_dd1, OUTPUT);
  Serial.begin(9600);
  delay(1000); // let serial console settle
  Wire.begin();
  RTC.begin();
  avr.begin();
  
  //RTC.adjust(DateTime(__DATE__, __TIME__));       // set RTC - uncomment only at first upload and comment for second upload
  DateTime now = RTC.now();
  st_Hour = now.hour();
  st_Minute = now.minute();

//  if (st_Minute > 30) {               // uncomment if log intervall is 30 min and logs should be at :00 and :30
//    Log_Hour = st_Hour +1;
//    Log_Minute = 0;
//  } else {
//    Log_Hour = st_Hour;
//    Log_Minute = 30;
//  }

//  Log_Hour = st_Hour + 1;             // uncomment if logging should start at :00 (next full hour)
//  Log_Minute = 0;

  Log_Hour = st_Hour;                   // logging starts within a minute
  Log_Minute = st_Minute + 1;

  Serial.print(st_Hour);                // just to check if RTC is set right
  Serial.print(":");
  Serial.println(st_Minute);

  Serial.print("Initializing SD card...");    // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  measure();                            // make a measurement at logger start outside logging routine
  
  delay(2000);

  digitalWrite(enable_rtc, LOW);        // disable RTC voltage supply

}

void loop() {
  
  digitalWrite(enable_rtc, HIGH);       // enable RTC voltage supply
  DateTime now = RTC.now();
  if(Log_Hour == now.hour() && Log_Minute == now.minute()) {          // check if current time equals time to log
    measure();                          // execute logging routine
    
    Log_Minute += Log_Interval_Minutes;       // set next time to log

    if (Log_Minute >= 60) {                   // adjust minute count if reaches 60
      Log_Minute -= 60;
      Log_Hour += 1;
      
      if (Log_Hour == 24) {                   // adjust hour count if reaches 24
        Log_Hour = 0;
                
      }
    }
  }

  RTC.disableInterrupts();                // clear RTC interrupts
  RTC.clearINTStatus(); 

  RTC.enableInterrupts(Log_Hour, Log_Minute,0);   // set RTC interrupt to next due time

  delay(1000);                            // delete ?????
  digitalWrite(enable_rtc, LOW);          // disable RTC voltage supply
  sleepNow();                             // enter sleep mode (will be waked up from RTC at time to log)
  
  

}

void measure() {
  DateTime now = RTC.now();               
  delay(200);
  SD.begin(chipSelect);                   // initiate comunication to SD-Card
  
  filename = String("Log0001" + String(i_file) + ".txt");       // filename: "Logger_name" + appendix
  dataFile = SD.open(filename.c_str(), FILE_WRITE);             // open file in write mode
    
  delay(50);
           
  digitalWrite(enable_temp, HIGH);        // enable voltage supply to DS18S20           
  digitalWrite(enable_dd1, HIGH);         // enable voltage supply to Dendrometer #1

  delay(100);

  // meassure Dendrometer #1
  mcp342x.setConf(addr, 1, 0, mode, rate, gain); //Measure total voltage (channel 0)
  delay(300);                                   // Adjust if needed!!!!
  vdd1 = mcp342x.getData(addr);
  delay(50);

  mcp342x.setConf(addr, 1, 1, mode, rate, gain); //Measure partial voltage (channel 1)
  delay(300);
  dd1 = mcp342x.getData(addr) / vdd1;
  dtostrf(dd1, 5, 4, str_dd1);
  delay(50);

  // meassure Dendrometer #2
//  mcp342x.setConf(addr, 1, 2, mode, rate, gain); //Measure total voltage (channel 2)
//  delay(250);
//  vdd2 = mcp342x.getData(addr);
//  delay(50);
//
//  mcp342x.setConf(addr, 1, 3, mode, rate, gain); //Measure partial voltage (channel 3)
//  delay(250);
//  dd2 = mcp342x.getData(addr) / vdd2;
//  dtostrf(dd2, 5, 4, str_dd2);
//  delay(50);
  

  batV = avr.getAVRVcc() / 1000;      // meassure battery/supply voltage in V
  dtostrf(batV, 4, 3, str_batV);

  sensors.requestTemperatures();      // meassure temperature
  delay(500);
  temp = sensors.getTempCByIndex(0);  // sensor at address 0

  mcp342x.setConf(addr, 1, 2, mode, rate, 3); //Measure voltage of channel 2 (e.g. for thermocouple)
  delay(300);
  xyl1 = mcp342x.getData(addr) * 1000000 / 40;    // convert µV in deltaT
  dtostrf(xyl1, 5, 4, str_xyl1);
  delay(50);

  //mcp342x.setConf(addr, 1, 3, mode, rate, 3); //Measure voltage of channel 3 (e.g. for thermocouple)
  //delay(250);
  //xyl2 = mcp342x.getData(addr) * 1000000 /40;   // convert µV in deltaT
  //dtostrf(xyl2, 5, 4, str_xyl2);
  //delay(50);

  delay(100);
  

  digitalWrite(enable_temp,LOW);          // disable voltage supply to ADC
  digitalWrite(enable_dd1, LOW);          // disable voltage supply to ADC

  Filesize = dataFile.size();             // get size of file

  // if file larger than 1MB, create new file
  if (Filesize > 1000000 && dataFile) {
    i_file += 1;
    filename = String("Log0001" + String(i_file) + ".txt");
    dataFile = SD.open(filename.c_str(), FILE_WRITE);
    Filesize = dataFile.size();
  }

  if (Filesize == 0 && dataFile) {
    dataFile.println("Date,dd1,xyl1,temp,battV");         // if File is empty (new), create header (if appropiate add "dd2" or "xyl2")
  } else {
    
  }

  if (dataFile) {
    
    print2digitsf(now.year());
    dataFile.print("-");
    print2digitsf(now.month());
    dataFile.print("-");
    print2digitsf(now.day());
    dataFile.print(" ");
    print2digitsf(now.hour());
    dataFile.print(":");
    print2digitsf(now.minute());
    dataFile.print(":");
    print2digitsf(now.second());
    dataFile.print(",");
    dataFile.print(str_dd1);
    dataFile.print(",");
//    dataFile.print(str_dd2);
//    dataFile.print(",");
    dataFile.print(str_xyl1);
    dataFile.print(",");
    //dataFile.print(str_xyl2);
    //dataFile.print(",");
    dataFile.print(temp);
    dataFile.print(",");
    dataFile.println(str_batV);
    dataFile.close();
    
  } else {
    Serial.println("Error opening File");
    
  }

  print2digits(now.year());
  Serial.print("-");
  print2digits(now.month());
  Serial.print("-");
  print2digits(now.day());
  Serial.print(" ");
  print2digits(now.hour());
  Serial.print(":");
  print2digits(now.minute());
  Serial.print(":");
  print2digits(now.second());
  Serial.print(",");
  Serial.print(str_dd1);
  Serial.print(",");
//  Serial.print(str_dd2);
//  Serial.print(",");
  Serial.print(str_xyl1);
  Serial.print(",");
  //Serial.print(str_xyl2);
  //Serial.print(",");
  Serial.print(temp);
  Serial.print(",");
  Serial.println(str_batV);
  
 
}

void sleepNow() {
// Set pin 2 as interrupt and attach handler:
    attachInterrupt(0, wakeup, LOW);
    delay(100);

    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // deep sleep  
}

void wakeup(void) {
  detachInterrupt(0); // detach interrupt on wakeup
  
  delay(500);

}

// functions to print numbers in date and time as two digits (e.g. 06 instead of 6)
void print2digitsf(int number) {
  if (number >= 0 && number < 10) {
    dataFile.write('0');
  }
  dataFile.print(number, DEC);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number, DEC);
}
