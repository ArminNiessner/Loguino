/*
 * Example sketch to take measurements of a photoresistor, a BME280 temperature, humidity and air 
 * pressure sensor, and a DS18S20 temperature sensor.
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
#include <SdFat.h>                      // lib for SD-Card
#include <ANiess.h>               // needed for RTC
#include <AN_RTClib.h>                     // lib for Real-Time-Clock
#include <LowPower.h>                   // lib for sleep mode/low power mode
#include <AVRUtils.h>                   // needed to messure supply voltage
#include <DallasTemperature.h>          // lib for DS18S20 Temperature sensor
#include <MCP342x.h>                    // lib for ADC
#include <BME280I2C.h>

#define ONE_WIRE_BUS 4                  // data pin for DS18S20
OneWire oneWire(ONE_WIRE_BUS);          
DallasTemperature sensors(&oneWire); 

AVRUtils avr;
DS3231 RTC;
BME280I2C bme;

SdFat SD;

#define SD_CS_PIN SS

File dataFile;
unsigned long Filesize;

const int enable_rtc = 5;               // pin to enable power supply to RTC
const int enable_temp = 6;              // pin to enable power supply to DS18S20
const int enable_dd1 = 8;               // pin to enable power supply to photo resistor

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
String app;
String filename;   
String file_ini = "Lg0001";                     
float vch1;                             // voltage ch1
float vrel;                              // 
char str_vrel[10] = "";
float batV;                             // Battery voltage, supply voltage
char str_batV[10] = "";

float bme_temp;
float bme_rh;
float bme_press;
float ds_temp;

void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = RTC.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour()-2, now.minute(), now.second());
}

void setup() {
  pinMode(enable_rtc, OUTPUT);          // enable RTC
  digitalWrite(enable_rtc, HIGH);
  pinMode(enable_temp, OUTPUT); 
  pinMode(enable_dd1, OUTPUT); 
  pinMode(wakePin, INPUT_PULLUP);
  Serial.begin(9600);
  delay(1000); // let serial console settle
  Wire.begin();
  RTC.begin();
  avr.begin();
  bme.begin();
  
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

//  Log_Minute = (st_Minute / 10 + 1) * 10; // uncomment if logging should start at next :00, :10, ;20, :30, :40, 50 minute
//  Log_Hour = st_Hour;
//  if (Log_Minute == 60) {
//    Log_Hour = st_Hour + 1;
//    Log_Minute = 0;
//  }

//  Log_Hour = st_Hour + 1;             // uncomment if logging should start at :00 (next full hour)
//  Log_Minute = 0;

  Log_Hour = st_Hour;                   // logging starts within a minute
  Log_Minute = st_Minute + 1;

  Serial.print("Current Time: ");
  Serial.print(st_Hour);                // just to check if RTC is set right
  Serial.print(":");
  Serial.println(st_Minute);
  Serial.print("Next scheduled log: ");
  Serial.print(Log_Hour);
  Serial.print(":");
  Serial.println(Log_Minute);
  
  Serial.print("Initializing SD card...");    // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  measure();                            // make a measurement at logger start outside logging routine
  
  delay(500);

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

  delay(500);                            // delete ?????
  digitalWrite(enable_rtc, LOW);          // disable RTC voltage supply
  sleepNow();                             // enter sleep mode (will be waked up from RTC at time to log)

}

void measure() {
  DateTime now = RTC.now();               
  delay(200);
  SD.begin(SD_CS_PIN);                   // initiate comunication to SD-Card
  bme.begin(); 
  delay(50);
  
  digitalWrite(enable_temp, HIGH);        // enable voltage supply to DS18S20
  digitalWrite(enable_dd1, HIGH);         // enable voltage supply to photoresistor

  delay(100);

  // meassure radiance
  mcp342x.setConf(addr, 1, 0, mode, rate, gain); //Measure total voltage (channel 0)
  delay(300);                                   // Adjust if needed!!!!
  vch1 = mcp342x.getData(addr);
  delay(50);

  mcp342x.setConf(addr, 1, 1, mode, rate, gain); //Measure partial voltage (channel 1)
  delay(300);
  vrel = mcp342x.getData(addr) / vch1;
  dtostrf(vrel, 5, 4, str_vrel);
  delay(50);

  batV = avr.getAVRVcc() / 1000;      // meassure battery/supply voltage in V
  dtostrf(batV, 4, 3, str_batV);

  sensors.requestTemperatures();      // meassure temperature
  delay(700);
  ds_temp = sensors.getTempCByIndex(0);  // sensor at address 0

  bme_temp = bme.temp();
  bme_rh = bme.hum();
  bme_press = bme.pres();

  delay(100);

  digitalWrite(enable_temp,LOW);         
  digitalWrite(enable_dd1, LOW);         

  app = print2digitsstr(i_file);
  filename = String(file_ini + app + ".txt");

  SdFile::dateTimeCallback(dateTime);
  dataFile = SD.open(filename.c_str(), FILE_WRITE);             // open file in write mode
  Filesize = dataFile.size();             // get size of file
  
  if (Filesize > 1000000 && dataFile) {
    dataFile.close();
    i_file += 1;
    app = print2digitsstr(i_file);
    filename = String(file_ini + app + ".txt");
    SdFile::dateTimeCallback(dateTime);
    dataFile = SD.open(filename.c_str(), FILE_WRITE);
    Filesize = dataFile.size();
  }
    
  if (Filesize == 0 && dataFile) {
    dataFile.println("Date,Rad,bme_temp,bme_rh,bme_pres,ds_temp,battV");         // if File is empty (new), create header (if appropiate add "dd2" or "xyl2")
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
    dataFile.print(str_vrel);
    dataFile.print(",");
    dataFile.print(bme_temp);
    dataFile.print(",");
    dataFile.print(bme_rh);
    dataFile.print(",");
    dataFile.print(bme_press);
    dataFile.print(",");
    dataFile.print(ds_temp);
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
  Serial.print(str_vrel);
  Serial.print(",");
  Serial.print(bme_temp);
  Serial.print(",");
  Serial.print(bme_rh);
  Serial.print(",");
  Serial.print(bme_press);
  Serial.print(",");
  Serial.print(ds_temp);
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

String print2digitsstr(int number) {
  String result;
  if (number >= 0 && number < 10) {
    result = "0" + String(number);
  } else {
    result = String(number);
  }
  return result;
}
