#include "SmallRTC.h"
/* SmallRTC by GuruSR (https://www.github.com/GuruSR/SmallRTC)
 * Originally forked from WatchyRTC with a variety of fixes and improvements.
 *
 * Version 1.0, January    2, 2022
 * Version 1.1, January    4, 2022 : Correct Months from 1 to 12 to 0 to 11.
 * Version 1.2, January    7, 2022 : Added atMinuteWake to enable minute based wakeup.
 * Version 1.3, January   28, 2022 : Corrected atMinuteWake missing AlarmInterrupt & moved ClearAlarm around.
 * Version 1.4, January   29, 2022 : Added Make & Break Time functions to MATCH TimeLib & time.h by reducing Month and Wday.
 * Version 1.5, January   30, 2022 : Fixed atMinuteWake to require extra values for DS3231M to work properly.
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
RTC_DATA_ATTR uint8_t m_rtctype;
RTC_DATA_ATTR uint32_t m_adc_pin;
RTC_DATA_ATTR bool b_operational;
RTC_DATA_ATTR float f_watchyhwver;
RTC_DATA_ATTR bool b_forceesp32;
RTC_DATA_ATTR gsrdrift __srtcdrift;

SmallRTC::SmallRTC() {
}

struct timeval tv;

void
SmallRTC::init() {
    uint8_t controlReg, mask;

    m_rtctype = RTC_UNKNOWN;
    m_adc_pin = 0;
    f_watchyhwver = 0;
    __srtcdrift.esprtc.drift = 0;
    __srtcdrift.extrtc.drift = 0;
    __srtcdrift.esprtc.begin = 0;
    __srtcdrift.extrtc.begin = 0;
    __srtcdrift.esprtc.slush = 0;
    __srtcdrift.extrtc.slush = 0;
    __srtcdrift.esprtc.fast = false;
    __srtcdrift.extrtc.fast = false;

#ifndef SMALL_RTC_NO_INT
    esp_chip_info_t chip_info[sizeof(esp_chip_info_t)];
    esp_chip_info(chip_info);
    if (chip_info->model == CHIP_ESP32C3) {
        m_rtctype = RTC_ESP32;
        b_operational = true;
        m_adc_pin = 9;
        f_watchyhwver = 3.0;
    } else {
#else
        if (1) {
#endif
        Wire.begin();
#ifndef SMALL_RTC_NO_DS3232
        Wire.beginTransmission(RTC_DS_ADDR);

        if (!Wire.endTransmission()) {
            m_rtctype = RTC_DS3231;
            m_adc_pin = 33;
            f_watchyhwver = 1.0;
            controlReg = rtc_ds.readRTC(0x0E);
            mask = _BV (7);

            if (controlReg & mask) {
                controlReg &= ~mask;
                rtc_ds.writeRTC(0x0E, controlReg);
            }

            b_operational = true;
            checkStatus(b_operational);
            rtc_ds.squareWave(DS3232RTC::SQWAVE_NONE);    //disable square wave output
            rtc_ds.alarm(DS3232RTC::ALARM_2);
            rtc_ds.setAlarm(DS3232RTC::ALM2_EVERY_MINUTE, 0, 0, 0, 0);    //alarm wakes up Watchy every minute
            rtc_ds.alarmInterrupt(DS3232RTC::ALARM_2, true);    //enable alarm interrupt
        } else {
#endif
#ifndef SMALL_RTC_NO_PCF8563
            Wire.beginTransmission(RTC_PCF_ADDR);
            if (!Wire.endTransmission()) {
                m_rtctype = RTC_PCF8563;
                b_operational = true;
                rtc_pcf.clearStatus();
                Wire.beginTransmission(0xa3 >> 1);
                Wire.write(0x09);
                Wire.write(0x80);
                Wire.write(0x80);
                Wire.write(0x80);
                Wire.write(0x80);
                Wire.write(0x0);
                Wire.write(0x0);
                Wire.endTransmission();
                if ((analogReadMilliVolts(34) / 500.0f) > 2) {
                    m_adc_pin = 34;
                    f_watchyhwver = 2.0;
                }                // Find the battery to determine hardware version.
                if ((analogReadMilliVolts(35) / 500.0f) > 2) {
                    m_adc_pin = 35;
                    f_watchyhwver = 1.5;
                }
            }
#endif
#ifndef SMALL_RTC_NO_DS3232
            }
#endif
        }
        if (f_watchyhwver == 0) {                            /* Try to find it by way of battery */
#ifndef SMALL_RTC_NO_DS3232
            if ((analogReadMilliVolts (33) / 500.0f) > 2)
            {
                m_adc_pin = 33;
                f_watchyhwver = 1.0;
            }
#endif
#ifndef SMALL_RTC_NO_PCF8563
            if ((analogReadMilliVolts(34) / 500.0f) > 2) {
                m_adc_pin = 34;
                f_watchyhwver = 2.0;
            }
            if ((analogReadMilliVolts(35) / 500.0f) > 2) {
                m_adc_pin = 35;
                f_watchyhwver = 1.5;
            }
#endif
        }
    }

void
SmallRTC::setDateTime(const String& datetime) {
    tmElements_t dateTimeParsed, testTime;
    uint8_t controlReg, mask;

    dateTimeParsed.Year = CalendarYrToTm(_getValue(datetime, ':', 0).toInt());    //YYYY - 1970
    dateTimeParsed.Month = _getValue(datetime, ':', 1).toInt();
    dateTimeParsed.Day = _getValue(datetime, ':', 2).toInt();
    dateTimeParsed.Hour = _getValue(datetime, ':', 3).toInt();
    dateTimeParsed.Minute = _getValue(datetime, ':', 4).toInt();
    dateTimeParsed.Second = _getValue(datetime, ':', 5).toInt();

#ifndef SMALL_RTC_NO_INT
    time_t t = SmallRTC::doMakeTime(dateTimeParsed);    //make and break to calculate dateTimeParsed.Wday
    SmallRTC::doBreakTime(t, dateTimeParsed);
    tv.tv_sec = t;
    settimeofday(&tv, 0);
    SmallRTC::driftReset(t, true);
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231) {
        dateTimeParsed.Wday++;
        rtc_ds.write(dateTimeParsed);
        SmallRTC::driftReset(t, false);
        controlReg = rtc_ds.readRTC(0x0E);
        mask = _BV (7);
        if (controlReg & mask) {
            controlReg &= ~mask;
            rtc_ds.writeRTC(0x0E, controlReg);
        }
        checkStatus();
        rtc_ds.read(testTime);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563) {
        //day, weekday, month, century(1=1900, 0=2000), year(0-99)
        rtc_pcf.setDate(dateTimeParsed.Day, dateTimeParsed.Wday, dateTimeParsed.Month, 0, tmYearToY2k (
                dateTimeParsed.Year));    //DS3231 has Wday range of 1-7, but TimeLib & PCF8563 require day of week in 0-6 range.
        //hr, min, sec
        rtc_pcf.setTime(dateTimeParsed.Hour, dateTimeParsed.Minute, dateTimeParsed.Second);
        rtc_pcf.clearStatus();
        testTime.Year = y2kYearToTm(rtc_pcf.getYear());
        testTime.Month = rtc_pcf.getMonth() - 1;
        testTime.Day = rtc_pcf.getDay();
        testTime.Hour = rtc_pcf.getHour();
        testTime.Minute = rtc_pcf.getMinute();
        SmallRTC::driftReset(t, false);
    }
    __srtcdrift.newmin = millis() + (60 - dateTimeParsed.Second) * 1000;
    if (b_operational) {
        b_operational = (dateTimeParsed.Year == testTime.Year && dateTimeParsed.Month == testTime.Month
                         && dateTimeParsed.Day == testTime.Day && dateTimeParsed.Hour == testTime.Hour
                         && dateTimeParsed.Minute == testTime.Minute);
    }
#endif
}

void
SmallRTC::read(tmElements_t &output) {
    SmallRTC::read(output, false);
}

void
SmallRTC::read(tmElements_t &output, bool internal) {
#ifndef SMALL_RTC_NO_INT
    tmElements_t ti;            /* Internal Only */
    gettimeofday(&tv, nullptr);
    SmallRTC::doBreakTime(tv.tv_sec, ti);
    checkStatus();
    if (m_rtctype == RTC_ESP32 || b_forceesp32 || internal) {
        __srtcdrift.newmin = millis() + (60 - ti.Second) * 1000;
        __srtcdrift.esprtc.drifted = false;
        SmallRTC::manageDrift(ti, true);
        output = ti;
    }
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231) {
        rtc_ds.read(output);
        __srtcdrift.newmin = millis() + (60 - output.Second) * 1000;
        output.Wday--;
        output.Month--;
        tv.tv_sec = SmallRTC::doMakeTime(ti);
        settimeofday(&tv, 0);
        __srtcdrift.extrtc.drifted = false;
        SmallRTC::manageDrift(ti, true);
        SmallRTC::manageDrift(output, false);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563) {
        output.Year = y2kYearToTm (rtc_pcf.getYear());
        output.Month = rtc_pcf.getMonth() - 1;
        output.Day = rtc_pcf.getDay();
        output.Wday = rtc_pcf.getWeekday();
        output.Hour = rtc_pcf.getHour();
        output.Minute = rtc_pcf.getMinute();
        output.Second = rtc_pcf.getSecond();
        __srtcdrift.newmin = millis() + (60 - output.Second) * 1000;
        tv.tv_sec = SmallRTC::doMakeTime(ti);
        settimeofday(&tv, nullptr);
        __srtcdrift.extrtc.drifted = false;
        SmallRTC::manageDrift(ti, true);
        SmallRTC::manageDrift(output, false);
    }
#endif
}

void
SmallRTC::set(tmElements_t input) {
    SmallRTC::set(input, false, false);
}

void
SmallRTC::set(tmElements_t tm, bool enforce, bool internal) {
    tmElements_t tst;
    bool bInt = (!enforce || (enforce && internal));
    uint8_t controlReg, mask;
    time_t t = SmallRTC::doMakeTime(tm);
    __srtcdrift.newmin = millis() + (60 - tm.Second) * 1000;
#ifndef SMALL_RTC_NO_INT
    if (bInt) {
        SmallRTC::driftReset(t, true);
        tv.tv_sec = t;
        settimeofday(&tv, nullptr);
    }
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (!(!enforce || (enforce && !internal))) {
        return;
    }

    if (m_rtctype == RTC_DS3231) {
        tm.Wday++;
        tm.Month++;
        rtc_ds.write(tm);
        SmallRTC::driftReset(t, false);
        controlReg = rtc_ds.readRTC(0x0E);
        mask = _BV (7);
        if (controlReg & mask) {
            controlReg &= ~mask;
            rtc_ds.writeRTC(0x0E, controlReg);
        }
        checkStatus();
        rtc_ds.read(tst);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563) {
        SmallRTC::doBreakTime(t, tm);    //make and break to calculate tm.Wday
        //day, weekday, month, century(1=1900, 0=2000), year(0-99)
        //DS3231 has Wday range of 1-7, but TimeLib & PCF8563 require day of week in 0-6 range.
        rtc_pcf.setDate(tm.Day, tm.Wday, tm.Month + 1, 0, tmYearToY2k(tm.Year));
        //hr, min, sec
        SmallRTC::driftReset(t, false);
        rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
        rtc_pcf.clearStatus();
        tst.Year = y2kYearToTm (rtc_pcf.getYear());
        tst.Month = rtc_pcf.getMonth() - 1;
        tst.Day = rtc_pcf.getDay();
        tst.Hour = rtc_pcf.getHour();
        tst.Minute = rtc_pcf.getMinute();
    }
#endif
    if (b_operational) {
        b_operational = (tm.Year == tst.Year && tm.Month == tst.Month
                         && tm.Day == tst.Day && tm.Hour == tst.Hour
                         && tm.Minute == tst.Minute);
    }
}

void
SmallRTC::driftReset(time_t t, bool internal) {
    gsrdrifting *g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;
    g->last = t;
    g->slush = 0;
}

void
SmallRTC::manageDrift(tmElements_t &input, bool internal) {
    double l = 0.0, d = 0.0;
    uint32_t v, s;
    time_t t = SmallRTC::doMakeTime(input);
    gsrdrifting *g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;
    s = 0;
    if (g->last == 0) {
        g->last = t;
    }
    if (g->drift) {
        l = t - (g->slush + g->last);
        s = floor(l / g->drift);
    }                            // Take the seconds and divide by Drift, rounding down.
    if (s > 0 && g->begin == 0) {                            // Drift offset needs to be dealt with.
        if (__srtcdrift.paused) {
            return;                    // Paused, don't process.
        }
        d = g->slush + (s * g->drift);    // Handle the seconds based on what happened.
        v = floor(d);
        t += ((g->fast ? -1 : 1) * s);
        SmallRTC::doBreakTime(t, input);
        SmallRTC::set(input, true, internal);
        g->last = t - (l - v);     // Set the current time with the addition of any leftover seconds.
        g->slush = (d - v);        // Put the leftovers back into the slush to account for drift in decimal.
        g->drifted = true;
    } else if (l <= 0.0) {
        g->last = t;                // Make sure that the last catches up in case things get turned on/off.
    }
}

void
SmallRTC::beginDrift(tmElements_t &input, bool internal) {
    gsrdrifting *g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;
    time_t T = SmallRTC::doMakeTime(input);

    if (g->begin == 0) {
        g->begin = T;
        g->drift = 0;
        g->fast = false;
        SmallRTC::set(input, true, internal);
    }
}

void
SmallRTC::pauseDrift(bool Pause) {
    __srtcdrift.paused = Pause;
}

void
SmallRTC::endDrift(tmElements_t &input, bool internal) {
    gsrdrifting *g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;
    time_t t = SmallRTC::doMakeTime(input);
    time_t o;
    tmElements_t oT;
    int64_t d;
    float f;
    SmallRTC::read(oT, internal);
    o = SmallRTC::doMakeTime(oT);
    if (g->begin != 0) {
        d = (t - o);
        f = o - g->begin;
        if (d != 0) {
            f = floor(((f / d) * 100.0));
            d = f;
            g->fast = (f < 0);
            g->drift = (g->fast ? 0 - d : d);
            g->drift /= 100.0;
        } else {
            g->drift = 0;
        }
        SmallRTC::set(input, true, internal);
        g->begin = 0;
    }
}

uint32_t
SmallRTC::getDrift(bool internal) {
    float T = (internal) ? __srtcdrift.esprtc.drift : __srtcdrift.extrtc.drift;
    uint32_t O;
    T *= 100.0;
    O = T;
    return O;
}

void
SmallRTC::setDrift(uint32_t drift, bool isFast, bool internal) {
    gsrdrifting *g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;
    float T = drift;
    g->drift = (T / 100.0);
    g->fast = isFast;
}

bool
SmallRTC::isFastDrift(bool internal) {
    return (internal) ? __srtcdrift.esprtc.fast : __srtcdrift.extrtc.fast;
}

bool
SmallRTC::isNewMinute() {
    if (millis() >= __srtcdrift.newmin) {
        __srtcdrift.newmin += 60000;
        return true;
    }
    return false;
}

bool
SmallRTC::updatedDrift(bool internal) {
    return (internal) ? __srtcdrift.esprtc.drifted : __srtcdrift.extrtc.drifted;
}

bool
SmallRTC::checkingDrift(bool internal) {
    return (internal) ? (__srtcdrift.esprtc.begin != 0) : (__srtcdrift.extrtc.begin != 0);
}

void
SmallRTC::clearAlarm() {
#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231) {
        rtc_ds.clearAlarm(DS3232RTC::ALARM_2);    //resets the alarm flag in the RTC
        return;
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563) {
        rtc_pcf.clearAlarm();
    }
#endif
}

void
SmallRTC::nextMinuteWake(bool enabled) {
    tmElements_t t;
    SmallRTC::read(t);
    atMinuteWake(t.Minute + 1, enabled);
}

void
SmallRTC::atTimeWake(uint8_t hour, uint8_t minute, bool enabled) {
    SmallRTC::atMinuteWake(hour, minute, enabled);
}

void
SmallRTC::atMinuteWake(uint8_t minute, bool enabled) {
    SmallRTC::atMinuteWake(99, minute, enabled);
}

void
SmallRTC::atMinuteWake(uint8_t hour, uint8_t minute, bool enabled) {
    tmElements_t t;
    int8_t X;
    time_t T;
    bool bHour;                    // Match Hour too.
#ifndef SMALL_RTC_NO_INT
    if (m_rtctype == RTC_ESP32 || b_forceesp32) {
        SmallRTC::read(t, true);
        X = (minute % 60) - t.Minute;
        if (X < 0)
            X += 60;
        T = ((X * 60) - t.Second) * 1000000;
        if (hour) {
            X = (hour % 24) - t.Hour;
            if (X < 0) {
                X += 24;
            }
            T += (X * 3600000000);
        }
        esp_sleep_enable_timer_wakeup(T);
    }
#endif
#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231) {
        //  uint8_t Hour, uint8_t DayOfWeek
        SmallRTC::read(t, false);

        if (!hour) {
            bHour = true;
            hour = t.Hour;
        } // 24:00 is midnight.

        if (minute < t.Minute || minute > 59) {
            hour++;
        }

        if (hour > 23 || hour < t.Hour) {
            t.Wday++;
        }

        rtc_ds.clearAlarm(DS3232RTC::ALARM_2);    //resets the alarm flag in the RTC
        rtc_ds.setAlarm(
                (bHour ? DS3232RTC::ALM2_MATCH_HOURS : DS3232RTC::ALM2_MATCH_MINUTES),
                (minute % 60), (hour % 24), (t.Wday % 7) + 1);
        rtc_ds.alarmInterrupt(DS3232RTC::ALARM_2, enabled);    // Turn interrupt on or off based on Enabled.
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563) {
        rtc_pcf.clearAlarm();

        if (hour) {
            hour %= 24;
        } else {
            hour = 99;
        }

        if (enabled) {
            rtc_pcf.setAlarm((minute % 60), hour, 99, 99);
        } else {
            rtc_pcf.resetAlarm();
        }
    }
#endif
}

uint8_t
SmallRTC::temperature() {
#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231) {
        return rtc_ds.temperature ();
    }
#endif
    return 255; //error
}

uint8_t
SmallRTC::getType() {
    return m_rtctype;
}

uint32_t
SmallRTC::getADCPin() {
    return m_adc_pin;
}

uint16_t
SmallRTC::getLocalYearOffset() {
    return 1900;
}

float
SmallRTC::getWatchyHWVer() {
    return f_watchyhwver;
}

void
SmallRTC::UseESP32(bool enforce) {
#ifndef SMALL_RTC_NO_INT
    if (m_rtctype != RTC_ESP32) {
        b_forceesp32 = enforce;
    }
#endif
}

bool
SmallRTC::OnESP32() {
    if (m_rtctype != RTC_ESP32) {
        return b_forceesp32;
    }

    return false;
}

time_t
SmallRTC::doMakeTime(tmElements_t tminput) {
    tminput.Month++;
    tminput.Wday++;

    return makeTime(tminput);
}

void
SmallRTC::doBreakTime(time_t &T, tmElements_t &p_tminout) {
    breakTime(T, p_tminout);

    p_tminout.Month--;
    p_tminout.Wday--;
}

bool
SmallRTC::isOperating() {
    return b_operational || m_rtctype == RTC_ESP32 || b_forceesp32;
}

void
SmallRTC::checkStatus(bool resetOp) {
#ifndef SMALL_RTC_NO_DS3232
    if (b_operational && m_rtctype == RTC_DS3231)
    {
        b_operational = !rtc_ds.oscStopped (resetOp);
    }
#endif
}

float
SmallRTC::getRTCBattery(bool critical) {
#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_PCF8563) {
        return (critical ? 3.45 : 3.58);
    }
#endif
#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_DS3231) {
        return (critical ? 3.65 : 3.69);    // 3.69 : 3.75 (TEST)
    }
#endif
    return (critical ? 3.45 : 3.49);
}

String
SmallRTC::_getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
