
/*
 * Example sketch to take measurements of a BME280 temperature, humidity and air pressure sensor. 
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
#include "SdFat.h"                      // lib for SD-Card
#include <ANiess.h>               // needed for RTC
#include <AN_RTClib.h>                     // lib for Real-Time-Clock
#include <LowPower.h>                   // lib for sleep mode/low power mode
#include <AVRUtils.h>                   // needed to messure supply voltage 
#include <BME280I2C.h>

BME280I2C bme;
AVRUtils avr;
DS3231 RTC;

SdFat SD;

#define SD_CS_PIN SS

File dataFile;
unsigned long Filesize;

const int enable_rtc = 5;               // pin to enable power supply to RTC

uint8_t Log_Hour;                       // variable for hour to log
uint8_t Log_Minute;                     // variable for minute to log
uint8_t st_Hour;                        // variable for hour at logger start
uint8_t st_Minute;                      // variable for minute at logger start
uint8_t Log_Interval_Minutes = 1;       // logging intervall in minutes

int wakePin = 2;                 // pin used for waking up

int i_file = 0;                         // initial file appendix (a new file will be created for every 1MB file size)
String app;
String filename;   
String file_ini = "Lg0001";             // set to serial number of the Loguino    

float bme_temp;
float bme_rh;
float bme_press;

float batV;                             // Battery voltage, supply voltage
char str_batV[10] = "";

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
  pinMode(wakePin, INPUT_PULLUP);
  Serial.begin(9600);
  delay(1000); // let serial console settle
  Wire.begin();
  RTC.begin();
  avr.begin();
  
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

  delay(500);                            
  digitalWrite(enable_rtc, LOW);          // disable RTC voltage supply
  sleepNow();                             // enter sleep mode (will be waked up from RTC at time to log)
}

void measure() {
  DateTime now = RTC.now();               
  delay(200);
  SD.begin(SD_CS_PIN);                   // initiate comunication to SD-Card

  delay(50);

  bme_temp = bme.temp();
  bme_rh = bme.hum();
  bme_press = bme.pres();
  
  batV = avr.getAVRVcc() / 1000;      // meassure battery/supply voltage in V
  dtostrf(batV, 4, 3, str_batV);

  delay(100);              

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
    dataFile.println("Date,bme_temp,bme_rh,bme_press,battV");         // if File is empty (new), create header
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
    dataFile.print(bme_temp);
    dataFile.print(",");
    dataFile.print(bme_rh);
    dataFile.print(",");
    dataFile.print(bme_press);
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
  Serial.print(bme_temp);
  Serial.print(",");
  Serial.print(bme_rh);
  Serial.print(",");
  Serial.print(bme_press);
  Serial.print(",");
  Serial.println(str_batV);
}

void sleepNow() {
    attachInterrupt(0, wakeup, LOW);
    delay(100);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // deep sleep  
}

void wakeup(void) {
  detachInterrupt(0); // detach interrupt on wakeup 
  delay(100);
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
