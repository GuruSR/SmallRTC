#ifndef SMALL_RTC_H
#define SMALL_RTC_H

/* SmallRTC by GuruSR (https://www.github.com/GuruSR/SmallRTC)
 * Originally forked from WatchyRTC with a variety of fixes and improvements.
 * 
 * Version 1.0, January  2, 2022
 * Version 1.1, January  4, 2022 : Correct Months from 1 to 12 to 0 to 11.
 * Version 1.2, January  7, 2022 : Added atMinuteWake to enable minute based wakeup.
 * Version 1.3, January 28, 2022 : Corrected atMinuteWake missing AlarmInterrupt & moved ClearAlarm around.
 * Version 1.4, January 29, 2022 : Added Make & Break Time functions to MATCH TimeLib & time.h by reducing Month and Wday.
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

#include <Timelib.h>
#include <DS3232RTC.h>
#include <Rtc_Pcf8563.h>

#define Unknown 0
#define DS3231 1
#define PCF8563 2
#define RTC_DS_ADDR 0x68
#define RTC_PCF_ADDR 0x51
#define RTC_LOCALYEAR_OFFSET 1900

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
        void resetWake();
        void nextMinuteWake(bool Enabled = true);
        void atMinuteWake(int Minute, bool Enabled = true);
        uint8_t temperature();
        uint8_t getType();
        uint32_t getADCPin();
        time_t MakeTime(tmElements_t TM);
        void BreakTime(time_t &T, tmElements_t &TM);
    private:
        String _getValue(String data, char separator, int index);
};
#endif
