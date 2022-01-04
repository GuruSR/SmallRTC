#include "SmallRTC.h"

/* Olson2POSIX by GuruSR (https://www.github.com/GuruSR/SmallRTC)
 * Originally forked from WatchyRTC with a variety of fixes and improvements.
 * 
 * Version 1.0, January 2, 2022
 * Version 1.1, January 4, 2022 : Correct Months from 1 to 12 to 0 to 11.
 *
 * This library offers an alternative to the WatchyRTC library, but also provides a 100% time.h and timelib.h
 * compliant RTC library.
 *
 * This library is adapted to work with the Arduino ESP32 and any other project that has similar libraries.
 *
 * MIT License
 *
 * Copyright (c) 2022 GuruSR
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

RTC_DATA_ATTR uint8_t RTCType;
RTC_DATA_ATTR uint32_t ADC_PIN;

SmallRTC::SmallRTC()
    : rtc_ds(false) {}

void SmallRTC::init(){
    RTCType = Unknown;
    ADC_PIN = 0;
    Wire.beginTransmission(RTC_DS_ADDR);
    if(Wire.endTransmission() == 0) {
        RTCType = DS3231; ADC_PIN = 33;
        rtc_ds.squareWave(SQWAVE_NONE); //disable square wave output
        rtc_ds.alarm(ALARM_2);
        rtc_ds.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
        rtc_ds.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
    }else{
        Wire.beginTransmission(RTC_PCF_ADDR);
        if(Wire.endTransmission() == 0) { RTCType = PCF8563; ADC_PIN = 35; }
    }
}

void SmallRTC::setDateTime(String datetime){
    tmElements_t tm;
    tm.Year = CalendarYrToTm(_getValue(datetime, ':', 0).toInt()); //YYYY - 1970
    tm.Month = _getValue(datetime, ':', 1).toInt() - 1;
    tm.Day = _getValue(datetime, ':', 2).toInt();
    tm.Hour = _getValue(datetime, ':', 3).toInt();
    tm.Minute = _getValue(datetime, ':', 4).toInt();
    tm.Second = _getValue(datetime, ':', 5).toInt();
    time_t t = makeTime(tm);  //make and break to calculate tm.Wday
    breakTime(t, tm);
    if (RTCType == DS3231){
        tm.Wday++;
        tm.Month++;
        rtc_ds.write(tm);
    }else if (RTCType == PCF8563){
        //day, weekday, month, century(1=1900, 0=2000), year(0-99)
        rtc_pcf.setDate(tm.Day, tm.Wday, tm.Month + 1, 0, tmYearToY2k(tm.Year)); //TimeLib & DS3231 has Wday range of 1-7, but PCF8563 stores day of week in 0-6 range
        //hr, min, sec
        rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
     }
}

void SmallRTC::read(tmElements_t &tm){
    if (RTCType == DS3231){
        rtc_ds.read(tm);
        tm.Wday--;
        tm.Month--;
    }else if (RTCType == PCF8563){
        tm.Year = y2kYearToTm(rtc_pcf.getYear());
        tm.Month = rtc_pcf.getMonth() - 1;
        tm.Day = rtc_pcf.getDay();
        tm.Wday = rtc_pcf.getWeekday();
        tm.Hour = rtc_pcf.getHour();
        tm.Minute = rtc_pcf.getMinute();
        tm.Second = rtc_pcf.getSecond();
    }    
}

void SmallRTC::set(tmElements_t tm){
    if(RTCType == DS3231){
        tm.Wday++;
        tm.Month++;
        rtc_ds.write(tm);
    }else if (RTCType == PCF8563){
        time_t t = makeTime(tm); //make and break to calculate tm.Wday
        breakTime(t, tm);
        //day, weekday, month, century(1=1900, 0=2000), year(0-99)
        rtc_pcf.setDate(tm.Day, tm.Wday, tm.Month + 1, 0, tmYearToY2k(tm.Year)); //TimeLib & DS3231 has Wday range of 1-7, but PCF8563 stores day of week in 0-6 range
        //hr, min, sec
        rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
    }
}

void SmallRTC::resetWake(){
    if (RTCType == DS3231){
        rtc_ds.clearAlarm(ALARM_2); //resets the alarm flag in the RTC
    }else if (RTCType == PCF8563){
        rtc_pcf.clearAlarm(); //resets the alarm flag in the RTC
    }    
}

void SmallRTC::nextMinuteWake(bool Enabled){
    if (RTCType == DS3231){
        rtc_ds.clearAlarm(ALARM_2); //resets the alarm flag in the RTC
        rtc_ds.alarmInterrupt(ALARM_2, Enabled);  // Turn interrupt on or off based on Enabled.
    }else if (RTCType == PCF8563){
        rtc_pcf.clearAlarm(); //resets the alarm flag in the RTC
        if (Enabled) rtc_pcf.setAlarm(constrain((rtc_pcf.getMinute() + 1) , 0 , 59), 99, 99, 99);   //set alarm to trigger 1 minute from now
        else rtc_pcf.resetAlarm();
    }
}

uint8_t SmallRTC::temperature(){
    if(RTCType == DS3231) return rtc_ds.temperature();
	return 255; //error
}

uint8_t SmallRTC::getType(){ return RTCType; }
uint32_t SmallRTC::getADCPin(){ return ADC_PIN; }

String SmallRTC::_getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
