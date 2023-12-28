#ifndef SMALL_RTC_H
#define SMALL_RTC_H

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

#include <TimeLib.h>
#include <DS3232RTC.h>
#include <Rtc_Pcf8563.h>
#include <Wire.h>
#include <esp_system.h>
#include <time.h>

#define Unknown 0
#define DS3231 1
#define PCF8563 2
#define ESP32 3
#define RTC_PCF_ADDR 0x51
#define RTC_DS_ADDR 0x68

RTC_DATA_ATTR struct GSRDrifting final {
    float Drift;    // Drift value in seconds (with 2 decimal places).
    bool Fast;      // The drift is fast.
    double Slush;   // This is used to hold the leftover from above when whole numbers accumulate.
    time_t Last;    // Last time it was altered.
    time_t Begin;   // Used to determine when it was started (and if calculation is happening).
    bool Drifted;   // Means the RTC was changed due to drift.
};

RTC_DATA_ATTR struct GSRDrift final {
    GSRDrifting ESPRTC; // Drift value for the internal RTC.
    GSRDrifting EXTRTC; // Drift value for an Internal RTC (if one is working/present).
    uint64_t NewMin;    // Detect new minute based on last load using millis.
    bool Paused;        // Means something the user of this library is asking that no drift offsets happen during this time.
};

class SmallRTC {
    public:
        DS3232RTC rtc_ds;
        Rtc_Pcf8563 rtc_pcf;
    public:
        SmallRTC();
        void init();
        void setDateTime(String datetime);
        void read(tmElements_t &tm);
        void set(tmElements_t tm);
        void clearAlarm();
        void nextMinuteWake(bool Enabled = true);
        void atMinuteWake(uint8_t Minute, bool Enabled = true);
        uint8_t temperature();
        uint8_t getType();
        uint32_t getADCPin();
        uint16_t getLocalYearOffset();
        float getWatchyHWVer();
        void UseESP32(bool Enforce);
        bool OnESP32();
        time_t MakeTime(tmElements_t TM);
        void BreakTime(time_t &T, tmElements_t &TM);
        bool isOperating();
        float getRTCBattery(bool Critical = false);
        void beginDrift(tmElements_t &TM, bool Internal);
        void pauseDrift(bool Pause);
        void endDrift(tmElements_t &TM, bool Internal);
        uint32_t getDrift(bool Internal);
        void setDrift(uint32_t Drift, bool isFast, bool Internal);
        bool isFastDrift(bool Internal);
        bool isNewMinute();
        bool updatedDrift(bool Internal);
        bool checkingDrift(bool Internal);
    private:
        void set(tmElements_t tm, bool Enforce, bool Internal);
        void read(tmElements_t &tm, bool Internal);
        void driftReset(time_t t, bool Internal);
        void manageDrift(tmElements_t &TM, bool Internal);
        void checkStatus(bool ResetOP = false);
        String _getValue(String data, char separator, int index);
};
#endif
