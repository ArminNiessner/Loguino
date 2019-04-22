
#include <Wire.h>
#include <ANiess.h>
#include <AN_RTClib.h>

DS3231 RTC;

void setup () {
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();
    
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
}

void loop () {
    DateTime now = RTC.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since 2000 = ");
    Serial.print(now.get());
    Serial.print("s = ");
    Serial.print(now.get() / 86400L);
    Serial.println("d");
    
    Serial.println();
    delay(3000);
}
