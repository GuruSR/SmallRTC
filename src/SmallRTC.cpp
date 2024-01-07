#include "SmallRTC.h"

/* SmallRTC by GuruSR (https://www.github.com/GuruSR/SmallRTC)
 * Originally forked from WatchyRTC with a variety of fixes and improvements.
 *
 * Version 1.0, January   2, 2022
 * Version 1.1, January   4, 2022 : Correct Months from 1 to 12 to 0 to 11.
 * Version 1.2, January   7, 2022 : Added atMinuteWake to enable minute based wakeup.
 * Version 1.3, January  28, 2022 : Corrected atMinuteWake missing AlarmInterrupt & moved ClearAlarm around.
 * Version 1.4, January  29, 2022 : Added Make & Break Time functions to MATCH TimeLib & time.h by reducing Month and Wday.
 * Version 1.5, January  30, 2022 : Fixed atMinuteWake to require extra values for DS3231M to work properly.
 * Version 1.6, February 17, 2022 : Added isOperating for the DS3231M to detect if the Oscillator has stopped.
 * Version 1.7, March    15, 2022 : Added Status, Squarewave & Timer reset to init for PCF8563.
 * Version 1.8, March    29, 2022 : Added support for 2 variations of PCF8563 battery location.
 * Version 1.9, April     4, 2022 : Added support for DS3232RTC version 2.0 by customizing defines.
 * Version 2.0, April    30, 2022 : Removed Constrain which was causing 59 minute stall.
 * Version 2.1, May      30, 2022 : Fix PCF.
 * Version 2.2, May       5, 2023 : Added functionality to keep this version alive.
 * Version 2.3, December 17, 2023 : Added ESP32 internal RTC functionality instead of keeping in Active Mode, 32bit drift (in 100ths of a second).
 * Version 2.3.1 January  2, 2024 : Added #define limitations to remove RTC versions you don't want to support.
 * Version 2.3.2 January  6, 2024 : Added atTimeWake function for specifying the hour and minute to wake the RTC up.
 *
 * This library offers an alternative to the WatchyRTC library, but also provides a 100% time.h and timelib.h
 * compliant RTC library.
 *
 * This library is adapted to work with the Arduino ESP32 and any other project that has similar libraries.
 *
 * MIT License
 *
 * Copyright (c) 2023 GuruSR
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
RTC_DATA_ATTR bool Operational;
RTC_DATA_ATTR float WatchyHWVer;
RTC_DATA_ATTR bool ForceESP32;
RTC_DATA_ATTR GSRDrift SRTCDrift;

SmallRTC::SmallRTC() {}

struct timeval tv;

void SmallRTC::init(){
    uint8_t controlReg, mask;
    RTCType = Unknown;
    ADC_PIN = 0;
    WatchyHWVer = 0;
    SRTCDrift.ESPRTC.Drift = 0;
    SRTCDrift.EXTRTC.Drift = 0;
    SRTCDrift.ESPRTC.Begin = 0;
    SRTCDrift.EXTRTC.Begin = 0;
    SRTCDrift.ESPRTC.Slush = 0;
    SRTCDrift.EXTRTC.Slush = 0;
    SRTCDrift.ESPRTC.Fast = false;
    SRTCDrift.EXTRTC.Fast = false;
#ifndef SMALL_RTC_NO_INT
    esp_chip_info_t chip_info[sizeof(esp_chip_info_t)];
    esp_chip_info(chip_info);
    if (chip_info->model == CHIP_ESP32C3) { RTCType = ESP32; Operational = true; ADC_PIN = 9; WatchyHWVer = 3.0; }
    else{
#else
    if (1){
#endif
        Wire.begin();
#ifndef SMALL_RTC_NO_DS3232
        Wire.beginTransmission(RTC_DS_ADDR);
        if(!Wire.endTransmission()) {
            RTCType = DS3231; ADC_PIN = 33; WatchyHWVer = 1.0;
            controlReg = rtc_ds.readRTC(0x0E);
            mask = _BV(7);
            if (controlReg & mask){
                controlReg &= ~mask;
                rtc_ds.writeRTC(0x0E, controlReg);
            }
            Operational = true;
            checkStatus(Operational);
            rtc_ds.squareWave(DS3232RTC::SQWAVE_NONE); //disable square wave output
            rtc_ds.alarm(DS3232RTC::ALARM_2);
            rtc_ds.setAlarm(DS3232RTC::ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
            rtc_ds.alarmInterrupt(DS3232RTC::ALARM_2, true); //enable alarm interrupt
        }else{
#endif
#ifndef SMALL_RTC_NO_PCF8563
        Wire.beginTransmission(RTC_PCF_ADDR);
            if(!Wire.endTransmission()) {
                RTCType = PCF8563; Operational = true; rtc_pcf.clearStatus(); Wire.beginTransmission(0xa3>>1); Wire.write(0x09); Wire.write(0x80); Wire.write(0x80); Wire.write(0x80); Wire.write(0x80); Wire.write(0x0); Wire.write(0x0); Wire.endTransmission();
                if ((analogReadMilliVolts(34) / 500.0f) > 1) { ADC_PIN = 34; WatchyHWVer = 2.0; }	// Find the battery to determine hardware version.
                if ((analogReadMilliVolts(35) / 500.0f) > 1) { ADC_PIN = 35; WatchyHWVer = 1.5; }
            }
#endif
#ifndef SMALL_RTC_NO_DS3232
        }
#endif
    }
    if (WatchyHWVer == 0){ /* Try to find it by way of battery */
#ifndef SMALL_RTC_NO_DS3232
        if ((analogReadMilliVolts(33) / 500.0f) > 1) { ADC_PIN = 33; WatchyHWVer = 1.0; }
#endif
#ifndef SMALL_RTC_NO_PCF8563
        if ((analogReadMilliVolts(34) / 500.0f) > 1) { ADC_PIN = 34; WatchyHWVer = 2.0; }
        if ((analogReadMilliVolts(35) / 500.0f) > 1) { ADC_PIN = 35; WatchyHWVer = 1.5; }
#endif
    }
}

void SmallRTC::setDateTime(String datetime){
    tmElements_t tm, tst;
    uint8_t controlReg, mask;
    tm.Year = CalendarYrToTm(_getValue(datetime, ':', 0).toInt()); //YYYY - 1970
    tm.Month = _getValue(datetime, ':', 1).toInt();
    tm.Day = _getValue(datetime, ':', 2).toInt();
    tm.Hour = _getValue(datetime, ':', 3).toInt();
    tm.Minute = _getValue(datetime, ':', 4).toInt();
    tm.Second = _getValue(datetime, ':', 5).toInt();
#ifndef SMALL_RTC_NO_INT
    time_t t = SmallRTC::MakeTime(tm);  //make and break to calculate tm.Wday
    SmallRTC::BreakTime(t, tm);
    tv.tv_sec = t;
    settimeofday(&tv,0);
    SmallRTC::driftReset(t,true);
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (RTCType == DS3231){
        tm.Wday++;
        rtc_ds.write(tm);
        SmallRTC::driftReset(t,false);
        controlReg = rtc_ds.readRTC(0x0E);
        mask = _BV(7);
        if (controlReg & mask){
            controlReg &= ~mask;
            rtc_ds.writeRTC(0x0E, controlReg);
        }
        checkStatus();
        rtc_ds.read(tst);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (RTCType == PCF8563){
        //day, weekday, month, century(1=1900, 0=2000), year(0-99)
        rtc_pcf.setDate(tm.Day, tm.Wday, tm.Month, 0, tmYearToY2k(tm.Year)); //DS3231 has Wday range of 1-7, but TimeLib & PCF8563 require day of week in 0-6 range.
        //hr, min, sec
        rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
        rtc_pcf.clearStatus();
        tst.Year = y2kYearToTm(rtc_pcf.getYear());
        tst.Month = rtc_pcf.getMonth() - 1;
        tst.Day = rtc_pcf.getDay();
        tst.Hour = rtc_pcf.getHour();
        tst.Minute = rtc_pcf.getMinute();
        SmallRTC::driftReset(t,false);
     }
    SRTCDrift.NewMin = millis() + (60 - tm.Second) * 1000;
    if (Operational) Operational = (tm.Year == tst.Year && tm.Month == tst.Month && tm.Day == tst.Day && tm.Hour == tst.Hour && tm.Minute == tst.Minute);
#endif
}

void SmallRTC::read(tmElements_t &tm){ SmallRTC::read(tm,false); }

void SmallRTC::read(tmElements_t &tm, bool Internal){
#ifndef SMALL_RTC_NO_INT
    tmElements_t ti;    /* Internal Only */
    gettimeofday(&tv,0);
    SmallRTC::BreakTime(tv.tv_sec, ti);
	checkStatus();
    if (RTCType == ESP32 || ForceESP32 || Internal){
        SRTCDrift.NewMin = millis() + (60 - ti.Second) * 1000;
        SRTCDrift.ESPRTC.Drifted = false;
        SmallRTC::manageDrift(ti,true);
        tm=ti;
    }
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (RTCType == DS3231){
        rtc_ds.read(tm);
        SRTCDrift.NewMin = millis() + (60 - tm.Second) * 1000;
        tm.Wday--;
        tm.Month--;
        tv.tv_sec = SmallRTC::MakeTime(ti);
        settimeofday(&tv,0);
        SRTCDrift.EXTRTC.Drifted = false;
        SmallRTC::manageDrift(ti,true);
        SmallRTC::manageDrift(tm,false);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (RTCType == PCF8563){
        tm.Year = y2kYearToTm(rtc_pcf.getYear());
        tm.Month = rtc_pcf.getMonth() - 1;
        tm.Day = rtc_pcf.getDay();
        tm.Wday = rtc_pcf.getWeekday();
        tm.Hour = rtc_pcf.getHour();
        tm.Minute = rtc_pcf.getMinute();
        tm.Second = rtc_pcf.getSecond();
        SRTCDrift.NewMin = millis() + (60 - tm.Second) * 1000;
        tv.tv_sec = SmallRTC::MakeTime(ti);
        settimeofday(&tv,0);
        SRTCDrift.EXTRTC.Drifted = false;
        SmallRTC::manageDrift(ti,true);
        SmallRTC::manageDrift(tm,false);
    }
#endif
}

void SmallRTC::set(tmElements_t tm){ SmallRTC::set(tm, false, false); }

void SmallRTC::set(tmElements_t tm, bool Enforce, bool Internal){
    tmElements_t tst;
    bool Int = (!Enforce || (Enforce && Internal));
    uint8_t controlReg, mask;
    time_t t = SmallRTC::MakeTime(tm);
    SRTCDrift.NewMin = millis() + (60 - tm.Second) * 1000;
#ifndef SMALL_RTC_NO_INT
    if (Int){
        SmallRTC::driftReset(t,true);
        tv.tv_sec = t;
        settimeofday(&tv,0);
    }
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (!(!Enforce || (Enforce && !Internal))) return;
    if (RTCType == DS3231){
        tm.Wday++;
        tm.Month++;
        rtc_ds.write(tm);
        SmallRTC::driftReset(t, false);
        controlReg = rtc_ds.readRTC(0x0E);
        mask = _BV(7);
        if (controlReg & mask){
            controlReg &= ~mask;
            rtc_ds.writeRTC(0x0E, controlReg);
        }
        checkStatus();
        rtc_ds.read(tst);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (RTCType == PCF8563){
        SmallRTC::BreakTime(t, tm); //make and break to calculate tm.Wday
        //day, weekday, month, century(1=1900, 0=2000), year(0-99)
        rtc_pcf.setDate(tm.Day, tm.Wday, tm.Month + 1, 0, tmYearToY2k(tm.Year)); //DS3231 has Wday range of 1-7, but TimeLib & PCF8563 require day of week in 0-6 range.
        //hr, min, sec
        SmallRTC::driftReset(t,false);
        rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
        rtc_pcf.clearStatus();
        tst.Year = y2kYearToTm(rtc_pcf.getYear());
        tst.Month = rtc_pcf.getMonth() - 1;
        tst.Day = rtc_pcf.getDay();
        tst.Hour = rtc_pcf.getHour();
        tst.Minute = rtc_pcf.getMinute();
    }
#endif
    if (Operational) Operational = (tm.Year == tst.Year && tm.Month == tst.Month && tm.Day == tst.Day && tm.Hour == tst.Hour && tm.Minute == tst.Minute);
}

void SmallRTC::driftReset(time_t t, bool Internal){
    GSRDrifting * G;
    if (Internal) G = &SRTCDrift.ESPRTC; else G = &SRTCDrift.EXTRTC;
    G->Last = t; G->Slush = 0;
}

void SmallRTC::manageDrift(tmElements_t &TM, bool Internal){
    double L = 0.0, D = 0.0;
    uint32_t V, S;
    time_t T = SmallRTC::MakeTime(TM);
    GSRDrifting * G;
    S = 0;
    if (Internal) G = &SRTCDrift.ESPRTC; else G = &SRTCDrift.EXTRTC;
    if (G->Last == 0) G->Last = T;
    if (G->Drift) { L = T - (G->Slush + G->Last); S = floor(L / G->Drift); }  // Take the seconds and divide by Drift, rounding down.
    if (S > 0 && G->Begin == 0){                 // Drift offset needs to be dealt with.
        if (SRTCDrift.Paused) return;            // Paused, don't process.
        D = G->Slush + (S * G->Drift);           // Handle the seconds based on what happened.
        V = floor(D);
        T += ((G->Fast ? -1 : 1) * S);
        SmallRTC::BreakTime(T,TM);
        SmallRTC::set(TM,true,Internal);
        G->Last = T - (L - V);                   // Set the current time with the addition of any leftover seconds.
        G->Slush = (D - V);                      // Put the leftovers back into the slush to account for drift in decimal.
        G->Drifted = true;
    }else if (L <= 0.0) G->Last = T;  // Make sure that the last catches up incase things get turned on/off.
}

void SmallRTC::beginDrift(tmElements_t &TM, bool Internal){
    GSRDrifting * G;
    time_t T = SmallRTC::MakeTime(TM);
    if (Internal) G = &SRTCDrift.ESPRTC; else G = &SRTCDrift.EXTRTC;
    if (G->Begin == 0) { G->Begin = T; G->Drift = 0; G->Fast = false; SmallRTC::set(TM, true, Internal); }
}

void SmallRTC::pauseDrift(bool Pause){ SRTCDrift.Paused = Pause; }

void SmallRTC::endDrift(tmElements_t &TM, bool Internal){
    GSRDrifting * G;
    time_t T = SmallRTC::MakeTime(TM);
    time_t O;
    tmElements_t OT;
    int64_t D;
    float F;
    SmallRTC::read(OT,Internal);
    O = SmallRTC::MakeTime(OT);
    if (Internal) G = &SRTCDrift.ESPRTC; else G = &SRTCDrift.EXTRTC;
    if (G->Begin != 0){
        D = (T - O); F = O - G->Begin;
        if (D != 0){ F = floor(((F / D) * 100.0)); D = F; G->Fast = (F < 0); G->Drift = (G->Fast ? 0 - D : D); G->Drift /= 100.0; } else G->Drift = 0;
        SmallRTC::set(TM, true, Internal); G->Begin = 0;
    }
}

uint32_t SmallRTC::getDrift(bool Internal){
    float T;
    uint32_t O;
    if (Internal) T = SRTCDrift.ESPRTC.Drift; else T = SRTCDrift.EXTRTC.Drift;
    T *= 100.0;
    O = T;
    return O;
}

void SmallRTC::setDrift(uint32_t Drift, bool isFast, bool Internal){
    GSRDrifting * G;
    if (Internal) G = &SRTCDrift.ESPRTC; else G = &SRTCDrift.EXTRTC;
    float T = Drift;
    G->Drift = (T / 100.0); G->Fast = isFast;
}

bool SmallRTC::isFastDrift(bool Internal){
    if (Internal) return SRTCDrift.ESPRTC.Fast; else return SRTCDrift.EXTRTC.Fast;
}

bool SmallRTC::isNewMinute() { if (millis() >= SRTCDrift.NewMin) { SRTCDrift.NewMin += 60000; return true; } return false; }

bool SmallRTC::updatedDrift(bool Internal){ if (Internal) return SRTCDrift.ESPRTC.Drifted; else return SRTCDrift.EXTRTC.Drifted; }

bool SmallRTC::checkingDrift(bool Internal){ if (Internal) return (SRTCDrift.ESPRTC.Begin != 0); else return (SRTCDrift.EXTRTC.Begin != 0); }

void SmallRTC::clearAlarm(){
#ifndef SMALL_RTC_NO_DS3232
    if (RTCType == DS3231) rtc_ds.clearAlarm(DS3232RTC::ALARM_2); //resets the alarm flag in the RTC
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (RTCType == PCF8563) rtc_pcf.clearAlarm();
#endif
}

void SmallRTC::nextMinuteWake(bool Enabled){
    tmElements_t t;
    SmallRTC::read(t);
    atMinuteWake(t.Minute + 1,Enabled);
}

void SmallRTC::atTimeWake(uint8_t Hour, uint8_t Minute, bool Enabled){ SmallRTC::atMinuteWake(Hour,Minute,Enabled); }
void SmallRTC::atMinuteWake(uint8_t Minute, bool Enabled) { SmallRTC::atMinuteWake(0,Minute,Enabled); }
void SmallRTC::atMinuteWake(uint8_t Hour, uint8_t Minute, bool Enabled){
    tmElements_t t;
    int8_t X;
    time_t T;
    bool bHour; // Match Hour too.
#ifndef SMALL_RTC_NO_INT
    if (RTCType == ESP32 || ForceESP32){
        SmallRTC::read(t,true);
        X = (Minute % 60) - t.Minute; if (X < 0) X += 60;
        T = ((X * 60) - t.Second) * 1000000;
        if (Hour) { X = (Hour % 24) - t.Hour; if (X < 0) X += 24; T += (X * 3600000000); }
        esp_sleep_enable_timer_wakeup(T);
	}
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (RTCType == DS3231){
        //  uint8_t Hour, uint8_t DayOfWeek
        SmallRTC::read(t,false);
        if (!Hour) { bHour = true; Hour = t.Hour; }  // 24:00 is midnight.
        if (Minute < t.Minute || Minute > 59) Hour++;
        if (Hour > 23 || Hour < t.Hour) t.Wday++;
        rtc_ds.clearAlarm(DS3232RTC::ALARM_2); //resets the alarm flag in the RTC
        rtc_ds.setAlarm((bHour ? DS3232RTC::ALM2_MATCH_HOURS : DS3232RTC::ALM2_MATCH_MINUTES),(Minute % 60), (Hour % 24), (t.Wday % 7) + 1);
        rtc_ds.alarmInterrupt(DS3232RTC::ALARM_2, Enabled);  // Turn interrupt on or off based on Enabled.
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (RTCType == PCF8563){
        rtc_pcf.clearAlarm();
        if (Hour) Hour %= 24; else Hour = 99;
        if (Enabled) rtc_pcf.setAlarm((Minute % 60), Hour, 99, 99);
        else rtc_pcf.resetAlarm();
    }
#endif
}

uint8_t SmallRTC::temperature(){
#ifndef SMALL_RTC_NO_DS3232
    if(RTCType == DS3231) return rtc_ds.temperature();
#endif
    return 255; //error
}

uint8_t SmallRTC::getType(){ return RTCType; }
uint32_t SmallRTC::getADCPin(){ return ADC_PIN; }
uint16_t SmallRTC::getLocalYearOffset(){ return 1900; }
float SmallRTC::getWatchyHWVer(){ return WatchyHWVer; }
void SmallRTC::UseESP32(bool Enforce) {
#ifndef SMALL_RTC_NO_INT
 if (RTCType != ESP32) ForceESP32 = Enforce;
#endif
}
bool SmallRTC::OnESP32() { if (RTCType != ESP32) return ForceESP32; return false; }

time_t SmallRTC::MakeTime(tmElements_t TM){
    TM.Month++;
    TM.Wday++;
    return makeTime(TM);
}

void SmallRTC::BreakTime(time_t &T, tmElements_t &TM){
    breakTime(T,TM);
    TM.Month--;
    TM.Wday--;
}

bool SmallRTC::isOperating() { return Operational || RTCType == ESP32 || ForceESP32; }
void SmallRTC::checkStatus(bool ResetOP) {
#ifndef SMALL_RTC_NO_DS3232
    if (Operational && RTCType == DS3231) Operational = !rtc_ds.oscStopped(ResetOP);
#endif
}
float SmallRTC::getRTCBattery(bool Critical){
#ifndef SMALL_RTC_NO_DS3232
    if (RTCType == PCF8563) return (Critical ?  3.45 : 3.58);
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (RTCType == DS3231) return (Critical ? 3.65 : 3.69);	// 3.69 : 3.75 (TEST)
#endif
    return (Critical ? 3.45 : 3.49);
}

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
