#ifndef SMALL_RTC_H
#define SMALL_RTC_H

/* SmallRTC by GuruSR (https://www.github.com/GuruSR/SmallRTC)
 * Originally forked from WatchyRTC with a variety of fixes and improvements.
 *
 * Version 1.0, January    2, 2022
 * Version 1.1, January    4, 2022 : Correct Months from 1 to 12 to 0 to 11.
 * Version 1.2, January    7, 2022 : Added atminuteWake to enable minute based wakeup.
 * Version 1.3, January   28, 2022 : Corrected atminuteWake missing AlarmInterrupt & moved ClearAlarm around.
 * Version 1.4, January   29, 2022 : Added Make & Break Time functions to MATCH TimeLib & time.h by reducing Month and Wday.
 * Version 1.5, January   30, 2022 : Fixed atminuteWake to require extra values for DS3231M to work properly.
 * Version 1.6, February  17, 2022 : Added isOperating for the DS3231M to detect if the Oscillator has stopped.
 * Version 1.7, March     15, 2022 : Added Status, Squarewave & Timer reset to init for PCF8563.
 * Version 1.8, March     29, 2022 : Added support for 2 variations of PCF8563 battery location.
 * Version 1.9, April      4, 2022 : Added support for DS3232RTC version 2.0 by customizing defines.
 * Version 2.0, April     30, 2022 : Removed Constrain which was causing 59 minute stall.
 * Version 2.1, May       30, 2022 : Fix PCF.
 * Version 2.2, May        5, 2023 : Added functionality to keep this version alive.
 * Version 2.3, December  17, 2023 : Added ESP32 internal RTC functionality instead of keeping in Active Mode, 32bit drift (in 100ths of a second).
 * Version 2.3.1 January   2, 2024 : Added #define limitations to remove RTC versions you don't want to support.
 * Version 2.3.2 January   6, 2024 : Added atTimeWake function for specifying the hour and minute to wake the RTC up.
 * Version 2.3.3 January   7, 2024 : Arduino bump due to bug.
 * Version 2.3.4 February 11, 2024 : Fixed ESP32 define to be specific to RTC.
 * Version 2.3.5 March     5, 2024 : Fixed copy/paste error and beautified source files, unified rtc type defines.
 * Version 2.3.6 April     1, 2024 : Fixed atTimeWake and atMinuteWake to use new RTC_OMIT_HOUR define.
 * Version 2.3.7 July      1, 2024 : Added RTC32K support, force default of internal RTC when no version found.
 * Version 2.3.8 July     12, 2024 : Cleaned up atTimeWake and atMinuteWake for better minute rollover.
 * Version 2.3.9 July     15, 2024 : Repair _validateWake with respect to internal RTC usage.
 * Version 2.4.0 July     18, 2029 : Clean up of timeval to remove bleed from previous use.
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

// Choose your options by editing this file or adding the #defines below prior to this library.
//  #define SMALL_RTC_NO_DS3232
//  #define SMALL_RTC_NO_PCF8563
//  #define SMALL_RTC_NO_INT

#include <TimeLib.h>

#ifndef SMALL_RTC_NO_DS3232

#include <DS3232RTC.h>

#else

#pragma message "SmallRTC: No support for DS3231M"

#endif

#ifndef SMALL_RTC_NO_PCF8563

#include <Rtc_Pcf8563.h>

#else

#pragma message "SmallRTC: No support for PCF8563"

#endif

#ifdef SMALL_RTC_NO_INT

#pragma message "SmallRTC: No support for ESP32 RTC"

#endif

#include <Wire.h>

#include <Arduino.h>

#include <time.h>

#include "soc/rtc.h"

#include "esp_chip_info.h"


#define RTC_OMIT_HOUR 255

#define RTC_PCF_ADDR 0x51

#define RTC_DS_ADDR 0x68


#define    RTC_UNKNOWN 0

#define    RTC_DS3231 1

#define    RTC_PCF8563 2

#define    RTC_ESP32 3

struct gsrdrifting final
{

    float drift;            // Drift value in seconds (with 2 decimal places).
    bool fast;                // The drift is fast.
    double slush;            // This is used to hold the leftover from above when whole numbers accumulate.
    time_t last;            // Last time it was altered.
    time_t begin;            // Used to determine when it was started (and if calculation is happening).
    bool drifted;            // Means the RTC was changed due to drift.

};


struct gsrdrift final
{

    gsrdrifting esprtc;        // Drift value for the internal RTC.
    gsrdrifting extrtc;        // Drift value for an internal RTC (if one is working/present).
    uint64_t newmin;        // Detect new minute based on last load using millis.
    bool paused;            // Means something the user of this library is asking that no drift offsets happen during this time.

};


class SmallRTC
{

    public:
#ifndef SMALL_RTC_NO_DS3232
        DS3232RTC rtc_ds;

#endif

#ifndef SMALL_RTC_NO_PCF8563
        Rtc_Pcf8563 rtc_pcf;

#endif

    public:
        SmallRTC ();

        void init ();

        void setDateTime (String datetime);

        void read (tmElements_t & p_tmoutput);

        void set (tmElements_t tminput);

        void clearAlarm ();

        void nextMinuteWake (bool enabled = true);

        void atMinuteWake (uint8_t minute, bool enabled = true);

        void atTimeWake (uint8_t hour, uint8_t minute, bool enabled = true);

        uint8_t temperature ();

        uint8_t getType ();

        uint32_t getADCPin ();

        uint16_t getLocalYearOffset ();

        float getWatchyHWVer ();

        void useESP32 (bool enforce, bool need32K = false);

        bool onESP32 ();

        time_t doMakeTime (tmElements_t tminput);

        void doBreakTime (time_t & p_tinput, tmElements_t & p_tminout);

        bool isOperating ();

        float getRTCBattery (bool critical = false);

        void beginDrift (tmElements_t & p_tminput, bool internal = false);

        void pauseDrift (bool pause);

        void endDrift (tmElements_t & p_tminput, bool internal = false);

        uint32_t getDrift (bool internal = false);

        void setDrift (uint32_t Drift, bool isFast, bool internal = false);

        bool isFastDrift (bool internal = false);

        bool isNewMinute ();

        bool updatedDrift (bool internal = false);

        bool checkingDrift (bool internal = false);

        void use32K (bool active);

    private:
        void set (tmElements_t tm, bool enforce, bool internal);

        void read (tmElements_t & tm, bool internal);

        void driftReset (time_t t, bool internal);

        void manageDrift (tmElements_t & tm, bool internal);

        void checkStatus (bool reset_op = false);

        void atMinuteWake (uint8_t hour, uint8_t minute, bool enabled = true);

        bool _validateWakeup (int8_t & mins, int8_t & hours, tmElements_t & t_data, bool b_internal);

        String _getValue (String data, char separator, int index);

};

#endif