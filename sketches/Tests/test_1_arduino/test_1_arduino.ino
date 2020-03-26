/*
 * test sketch to check if Arduino Pro Mini is working properly
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
 
#include <LowPower.h> 

void setup()
{
  Serial.begin(9600);
  Serial.println("Start test"); 
}

void loop()
{
  delay(2000);
  Serial.println("Going to sleep....zzzzzzz");
  sleepNow();
}

void sleepNow() {
    delay(100);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // deep sleep  
}
