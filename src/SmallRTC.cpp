#include "SmallRTC.h"

/* SmallRTC by GuruSR (https://www.github.com/GuruSR/SmallRTC)
 * Originally forked from WatchyRTC with a variety of fixes and improvements.
 *
 * Version 1.0, January    2, 2022
 * Version 1.1, January    4, 2022 : Correct Months from 1 to 12 to 0 to 11.
 * Version 1.2, January    7, 2022 : Added atMinuteWake to enable minute based
 *                                   wakeup.
 * Version 1.3, January   28, 2022 : Corrected atMinuteWake missing
 *                                   AlarmInterrupt & moved ClearAlarm around.
 * Version 1.4, January   29, 2022 : Added Make & Break Time functions to
 *                                   MATCH TimeLib & time.h by reducing Month
 *                                   and Wday.
 * Version 1.5, January   30, 2022 : Fixed atMinuteWake to require extra
 *                                   values for DS3231M to work properly.
 * Version 1.6, February  17, 2022 : Added isOperating for the DS3231M to
 *                                   detect if the Oscillator has stopped.
 * Version 1.7, March     15, 2022 : Added Status, Squarewave & Timer reset
 *                                   to init for PCF8563.
 * Version 1.8, March     29, 2022 : Added support for 2 variations of PCF8563
 *                                   battery location.
 * Version 1.9, April      4, 2022 : Added support for DS3232RTC version 2.0
 *                                   by customizing defines.
 * Version 2.0, April     30, 2022 : Removed Constrain which was causing 59
 *                                   minute stall.
 * Version 2.1, May       30, 2022 : Fix PCF.
 * Version 2.2, May        5, 2023 : Added functionality to keep this version
 *                                   alive.
 * Version 2.3, December  17, 2023 : Added ESP32 internal RTC functionality
 *                                   instead of keeping in Active Mode, 32bit
 *                                   drift (in 100ths of a second).
 * Version 2.3.1 January   2, 2024 : Added #define limitations to remove RTC
 *                                   versions you don't want to support.
 * Version 2.3.2 January   6, 2024 : Added atTimeWake function for specifying
 *                                   the hour and minute to wake the RTC up.
 * Version 2.3.3 January   7, 2024 : Arduino bump due to bug.
 * Version 2.3.4 February 11, 2024 : Fixed ESP32 define to be specific to RTC.
 * Version 2.3.5 March     5, 2024 : Fixed copy/paste error and beautified
 *                                   source files, unified rtc type defines.
 * Version 2.3.6 April     1, 2024 : Fixed atTimeWake and atMinuteWake to use
 *                                   new RTC_OMIT_HOUR define.
 * Version 2.3.7 July      1, 2024 : Added RTC32K support, force default of
 *                                   internal RTC when no version found.
 * Version 2.3.8 July     12, 2024 : Cleaned up atTimeWake and atMinuteWake
 *                                   for better minute rollover.
 * Version 2.3.9 July     15, 2024 : Repair _validateWake with respect to
 *                                   internal RTC usage.
 * Version 2.4.0 July     18, 2024 : Clean up of timeval to remove bleed from
 *                                   previous use.
 * Version 2.4.1 July     22, 2024 : Moved RTC32K to init to run once.
 *                                   Removed year manipulation.
 * Version 2.4.2 July     25, 2024 : Corrected manageDrift to properly deal
 *                                   with leftovers and negative durations.
 * Version 2.4.3 July     30, 2024 : Fixed bizarre conversion from float to
 *                                   uint32_t causing a value to be off.
 *
 * This library offers an alternative to the WatchyRTC library, but also
 * provides a 100% time.h and timelib.h compliant RTC library.
 *
 * This library is adapted to work with the Arduino ESP32 and any other project
 * that has similar libraries.
 *
 * MIT License
 *
 * Copyright (c) 2023 GuruSR
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

RTC_DATA_ATTR uint8_t m_rtctype;

RTC_DATA_ATTR uint32_t m_adc_pin;

RTC_DATA_ATTR bool b_operational;

RTC_DATA_ATTR float f_watchyhwver;

RTC_DATA_ATTR bool b_forceesp32;

RTC_DATA_ATTR bool b_use32K;

RTC_DATA_ATTR uint8_t m_rtc_pin;

RTC_DATA_ATTR gsrdrift __srtcdrift;


SmallRTC::SmallRTC ()
{
}

struct timespec tv;


void
SmallRTC::init ()
{

    uint8_t controlReg, mask;

    m_rtctype = RTC_UNKNOWN;

    m_adc_pin = 0;

    m_rtc_pin = 0;

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
    esp_chip_info_t chip_info[sizeof (esp_chip_info_t)];

    esp_chip_info (chip_info);

    if (chip_info->model == CHIP_ESP32S3)
    {

        m_rtctype = RTC_ESP32;

        b_operational = true;

        m_adc_pin = 9;

        f_watchyhwver = 3.0;

        b_use32K = true;

    }

  else

    {

#else

    if (1)
    {

#endif
        Wire.begin ();

#ifndef SMALL_RTC_NO_DS3232
        Wire.beginTransmission (RTC_DS_ADDR);

        if (!Wire.endTransmission ())
        {

            m_rtctype = RTC_DS3231;

            m_adc_pin = 33;

            m_rtc_pin = 27;

            f_watchyhwver = 1.0;

            controlReg = rtc_ds.readRTC (0x0E);

            mask = _BV (7);

            if (controlReg & mask)
            {

                controlReg &= ~mask;

                rtc_ds.writeRTC (0x0E, controlReg);

            }

            b_operational = true;

            checkStatus (b_operational);

            rtc_ds.squareWave (DS3232RTC::SQWAVE_NONE);

            rtc_ds.alarm (DS3232RTC::ALARM_2);

            rtc_ds.setAlarm (DS3232RTC::ALM2_EVERY_MINUTE, 0, 0, 0, 0);

            rtc_ds.alarmInterrupt (DS3232RTC::ALARM_2, true);

        }
      else
        {

#endif

#ifndef SMALL_RTC_NO_PCF8563
            Wire.beginTransmission (RTC_PCF_ADDR);

            if (!Wire.endTransmission ())
            {

                m_rtctype = RTC_PCF8563;

                m_rtc_pin = 27;

                b_operational = true;

                rtc_pcf.clearStatus ();

                Wire.beginTransmission (0xa3 >> 1);

                Wire.write (0x09);

                Wire.write (0x80);

                Wire.write (0x80);

                Wire.write (0x80);

                Wire.write (0x80);

                Wire.write (0x0);

                Wire.write (0x0);

                Wire.endTransmission ();

                if ((analogReadMilliVolts (34) / 500.0f) > 2)
                {

                    m_adc_pin = 34;

                    f_watchyhwver = 2.0;

                } // Find the battery to determine hardware version.

                if ((analogReadMilliVolts (35) / 500.0f) > 2)
                {

                    m_adc_pin = 35;

                    f_watchyhwver = 1.5;

                }

            }

#endif

#ifndef SMALL_RTC_NO_DS3232
        }

#endif

    }

    if (!f_watchyhwver)
    {      /* Try to find it by way of battery */

#ifndef SMALL_RTC_NO_DS3232
        if ((analogReadMilliVolts (33) / 500.0f) > 2)
        {

            m_adc_pin = 33;

            m_rtc_pin = 27;

            f_watchyhwver = 1.0;

        }

#endif

#ifndef SMALL_RTC_NO_PCF8563
        if ((analogReadMilliVolts (34) / 500.0f) > 2)
        {

            m_adc_pin = 34;

            m_rtc_pin = 27;

            f_watchyhwver = 2.0;

        }

        if ((analogReadMilliVolts (35) / 500.0f) > 2)
        {

            m_adc_pin = 35;

            m_rtc_pin = 27;

            f_watchyhwver = 1.5;

        }

#endif

        if (!f_watchyhwver)
        {

            b_forceesp32 = true;

        }

        if (b_use32K)
        {

            rtc_clk_32k_enable(b_use32K);

        }

    }

}


void
SmallRTC::setDateTime (String datetime)
{

    tmElements_t tm, tst;

    uint8_t controlReg, mask;

    tm.Year = CalendarYrToTm (_getValue (datetime, ':', 0).toInt ());

    tm.Month = _getValue (datetime, ':', 1).toInt ();

    tm.Day = _getValue (datetime, ':', 2).toInt ();

    tm.Hour = _getValue (datetime, ':', 3).toInt ();

    tm.Minute = _getValue (datetime, ':', 4).toInt ();

    tm.Second = _getValue (datetime, ':', 5).toInt ();

#ifndef SMALL_RTC_NO_INT
    time_t t = SmallRTC::doMakeTime (tm);

    SmallRTC::doBreakTime (t, tm);

    tv.tv_nsec = 0;

    tv.tv_sec = t;

    clock_settime (CLOCK_REALTIME, &tv);

    SmallRTC::driftReset (t, true);

#endif

#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231)
    {

        tm.Wday++;

        rtc_ds.write (tm);

        SmallRTC::driftReset (t, false);

        controlReg = rtc_ds.readRTC (0x0E);

        mask = _BV (7);

        if (controlReg & mask)
        {

            controlReg &= ~mask;

            rtc_ds.writeRTC (0x0E, controlReg);

        }

    checkStatus ();

    rtc_ds.read (tst);

}

#endif

#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563)
    {

        rtc_pcf.setDate (tm.Day, tm.Wday, tm.Month, 0, tm.Year);

        rtc_pcf.setTime (tm.Hour, tm.Minute, tm.Second);

        rtc_pcf.clearStatus ();

        tst.Year = rtc_pcf.getYear ();

        tst.Month = rtc_pcf.getMonth () - 1;

        tst.Day = rtc_pcf.getDay ();

        tst.Hour = rtc_pcf.getHour ();

        tst.Minute = rtc_pcf.getMinute ();

        SmallRTC::driftReset (t, false);

    }

    __srtcdrift.newmin = millis () + (60 - tm.Second) * 1000;

    if (b_operational)
    {

        b_operational = (tm.Year == tst.Year && tm.Month == tst.Month
            && tm.Day == tst.Day && tm.Hour == tst.Hour
            && tm.Minute == tst.Minute);

    }

#endif

}


void
SmallRTC::read (tmElements_t & p_tmoutput)
{

    SmallRTC::read (p_tmoutput, false);

}

void

SmallRTC::read (tmElements_t & p_tmoutput, bool internal)
{

#ifndef SMALL_RTC_NO_INT
    tmElements_t ti;

    clock_gettime (CLOCK_REALTIME, &tv);

    SmallRTC::doBreakTime (tv.tv_sec, ti);

    checkStatus ();

    if (m_rtctype == RTC_ESP32 || b_forceesp32 || internal)
    {

        __srtcdrift.newmin = millis () + (60 - ti.Second) * 1000;

        __srtcdrift.esprtc.drifted = false;

        SmallRTC::manageDrift (ti, true);

        p_tmoutput = ti;

    }

#endif

#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231)
    {

        rtc_ds.read (p_tmoutput);

        __srtcdrift.newmin = millis () + (60 - p_tmoutput.Second) * 1000;

        p_tmoutput.Wday--;

        p_tmoutput.Month--;

        tv.tv_nsec = 0;

        tv.tv_sec = SmallRTC::doMakeTime (ti);

        clock_settime (CLOCK_REALTIME, &tv);

        __srtcdrift.extrtc.drifted = false;

        SmallRTC::manageDrift (ti, true);

        SmallRTC::manageDrift (p_tmoutput, false);

    }

#endif

#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563)
    {

        p_tmoutput.Year = rtc_pcf.getYear ();

        p_tmoutput.Month = rtc_pcf.getMonth () - 1;

        p_tmoutput.Day = rtc_pcf.getDay ();

        p_tmoutput.Wday = rtc_pcf.getWeekday ();

        p_tmoutput.Hour = rtc_pcf.getHour ();

        p_tmoutput.Minute = rtc_pcf.getMinute ();

        p_tmoutput.Second = rtc_pcf.getSecond ();

        __srtcdrift.newmin = millis () + (60 - p_tmoutput.Second) * 1000;

        tv.tv_nsec = 0;

        tv.tv_sec = SmallRTC::doMakeTime (ti);

        clock_settime (CLOCK_REALTIME, &tv);

        __srtcdrift.extrtc.drifted = false;

        SmallRTC::manageDrift (ti, true);

        SmallRTC::manageDrift (p_tmoutput, false);

    }

#endif

}


void
SmallRTC::set (tmElements_t tminput)
{

    SmallRTC::set (tminput, false, false);

}


void
SmallRTC::set (tmElements_t tm, bool enforce, bool internal)
{

    tmElements_t tst;

    uint8_t controlReg, mask;

    time_t t = SmallRTC::doMakeTime (tm);

    __srtcdrift.newmin = millis () + (60 - tm.Second) * 1000;

#ifndef SMALL_RTC_NO_INT
    if (!enforce || (enforce && internal))
    {

        tv.tv_nsec = 0;

        tv.tv_sec = t;

        clock_settime (CLOCK_REALTIME, &tv);

        SmallRTC::driftReset (t, true);

    }

#endif

    if (!(!enforce || (enforce && !internal)))
    {

        return;

    }

#ifndef SMALL_RTC_NO_DS3232

    if (m_rtctype == RTC_DS3231)
    {

        tm.Wday++;

        tm.Month++;

        rtc_ds.write (tm);

        SmallRTC::driftReset (t, false);

        controlReg = rtc_ds.readRTC (0x0E);

        mask = _BV (7);

        if (controlReg & mask)
        {

            controlReg &= ~mask;

            rtc_ds.writeRTC (0x0E, controlReg);

        }

        SmallRTC::checkStatus ();

        rtc_ds.read (tst);

    }

#endif

#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563)
    {

        SmallRTC::doBreakTime (t, tm);

        rtc_pcf.setDate (tm.Day, tm.Wday, tm.Month + 1, 0, tm.Year);

        SmallRTC::driftReset (t, false);

        rtc_pcf.setTime (tm.Hour, tm.Minute, tm.Second);

        rtc_pcf.clearStatus ();

        tst.Year = rtc_pcf.getYear ();

        tst.Month = rtc_pcf.getMonth () - 1;

        tst.Day = rtc_pcf.getDay ();

        tst.Hour = rtc_pcf.getHour ();

        tst.Minute = rtc_pcf.getMinute ();

    }

#endif

    if (b_operational)
    {

        b_operational = (tm.Year == tst.Year && tm.Month == tst.Month
            && tm.Day == tst.Day && tm.Hour == tst.Hour
            && tm.Minute == tst.Minute);

    }
}


void
SmallRTC::driftReset (time_t t, bool internal)
{

    gsrdrifting* g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;

    g->last = t;

    g->slush = 0;

}


void
SmallRTC::manageDrift (tmElements_t & p_tminput, bool internal)
{

    double l = 0.0, d = 0.0;

    uint32_t v, s;

    int32_t r;

    time_t t = SmallRTC::doMakeTime (p_tminput);

    gsrdrifting * g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;

    s = 0;

    if (g->last == 0)
    {

        g->last = t;

    }

    if (g->drift)
    {

        l = t - g->last;

        if (l > 0.0)
        {

            s = floor(l / g->drift); 

        }

    }     // Take the seconds and divide by Drift, rounding down.

    if (s > 0 && g->begin == 0)
    {     // Drift offset needs to be dealt with.

        if (__srtcdrift.paused)
        {

            return;  // Paused, don't process.

        }

        d = g->slush + (s * g->drift);           // Handle the seconds based on what happened.

        v = floor(d);

        r = ((g->fast ? -1 : 1) * s);

        t += r;

        SmallRTC::doBreakTime (t, p_tminput);

        SmallRTC::set (p_tminput, true, internal);

        g->last = t + (l - v);                   // Set the current time with the addition of any leftover seconds.

        g->slush = (d - v);                      // Put the leftovers back into the slush to account for drift in decimal.

        g->drifted = true;

    }
    else if (l <= 0.0)
    {

        // Make sure that the last catches up incase things get turned on/off.

        g->last = t;

    }

}


void
SmallRTC::beginDrift (tmElements_t & p_tminput, bool internal)
{

    if (!internal)
    {

        internal = b_forceesp32;

    }

    gsrdrifting * g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;

    if (g->begin == 0)
    {

        g->begin = SmallRTC::doMakeTime (p_tminput);

        g->drift = 0;

        g->fast = false;

        SmallRTC::set (p_tminput, true, internal);

    }

}


void
SmallRTC::pauseDrift (bool Pause)
{

    __srtcdrift.paused = Pause;

}


void
SmallRTC::endDrift (tmElements_t & p_tminput, bool internal)
{

    if (!internal)
    {

        internal = b_forceesp32;

    }

    gsrdrifting * g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;

    time_t t = SmallRTC::doMakeTime (p_tminput);

    time_t o;

    tmElements_t oT;

    int64_t d;

    float f;

    SmallRTC::read (oT, internal);

    o = SmallRTC::doMakeTime (oT);

    if (g->begin != 0)
    {

        d = (t - o);

        f = round (o - g->begin);

        if (d != 0)
        {

            f = floor (((f / d) * 100.0));

            d = f;

            g->fast = (f < 0);

            g->drift = (g->fast ? 0 - d : d);

            g->drift /= 100.0;

        }
        else
        {

            g->drift = 0;

        }

       SmallRTC::set (p_tminput, true, internal);

        g->begin = 0;

    }

}


uint32_t
SmallRTC::getDrift (bool internal)
{

    float t;

    if (!internal)
    {

        internal = b_forceesp32;

    }

    t = (internal) ? __srtcdrift.esprtc.drift : __srtcdrift.extrtc.drift;

    t *= 100.0;

    return (uint32_t)round (t);

}


void
SmallRTC::setDrift (uint32_t drift, bool isfast, bool internal)
{

    if (!internal)
    {

        internal = b_forceesp32;

    }

    gsrdrifting * g = (internal) ? &__srtcdrift.esprtc : &__srtcdrift.extrtc;

    float T = drift;

    g->drift = (T / 100.0);

    g->fast = isfast;

}


bool
SmallRTC::isFastDrift (bool internal)
{

    if (!internal)
    {

        internal = b_forceesp32;

    }

    return (internal) ? __srtcdrift.esprtc.fast : __srtcdrift.extrtc.fast;

}


bool
SmallRTC::isNewMinute ()
{

    if (millis () >= __srtcdrift.newmin)
    {

        __srtcdrift.newmin += 60000;

        return true;

    }
    return false;

}


bool
SmallRTC::updatedDrift (bool internal)
{

    if (!internal)
    {

        internal = b_forceesp32;

    }

    return (internal) ? __srtcdrift.esprtc.drifted :
                        __srtcdrift.extrtc.drifted;

}


bool
SmallRTC::checkingDrift (bool internal)
{

    if (!internal)
    {

        internal = b_forceesp32;

    }

    return (internal) ? (__srtcdrift.esprtc.begin != 0) :
                        (__srtcdrift.extrtc.begin != 0);

}


void
SmallRTC::clearAlarm ()
{

#ifndef SMALL_RTC_NO_DS3232
if (m_rtctype == RTC_DS3231)
    {

        rtc_ds.clearAlarm (DS3232RTC::ALARM_2);

        return;

    }
#endif

#ifndef SMALL_RTC_NO_PCF8563
if (m_rtctype == RTC_PCF8563)
    {

        rtc_pcf.clearAlarm ();

    }
#endif

}


void
SmallRTC::nextMinuteWake (bool enabled)
{

    tmElements_t t;

    SmallRTC::read (t);

    SmallRTC::atMinuteWake (t.Minute + 1, enabled);

}


void
SmallRTC::atTimeWake (uint8_t hour, uint8_t minute, bool enabled)
{

    SmallRTC::atMinuteWake (hour, minute, enabled);

}


void
SmallRTC::atMinuteWake (uint8_t minute, bool enabled)
{

    SmallRTC::atMinuteWake (RTC_OMIT_HOUR, minute, enabled);

}


void
SmallRTC::atMinuteWake (uint8_t hour, uint8_t minute, bool enabled)
{

    bool b;

    tmElements_t t;

    int8_t wantedHour, wantedMinute;

    uint64_t waitTime, workHour, workMin;

    workHour = 0;

    wantedHour = hour;

    wantedMinute = minute;

#ifndef SMALL_RTC_NO_INT
    if (m_rtctype == RTC_ESP32 || b_forceesp32)
    {

        b = SmallRTC::_validateWakeup (wantedMinute, wantedHour, t, true);

        if (hour != RTC_OMIT_HOUR)
        {

            workHour = (uint64_t)wantedHour;

            workHour *= 3600;

        }

        workMin = (uint64_t)wantedMinute;

        workMin *= 60;

        waitTime = ((workHour + workMin - t.Second) * 1000000);

        esp_sleep_enable_timer_wakeup (waitTime);

    }

#endif

#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231)
    {

        b = SmallRTC::_validateWakeup (wantedMinute, wantedHour, t, false);

        if (hour == RTC_OMIT_HOUR)
        {

            wantedHour = t.Hour;

        }

        if (b)
        {

            t.Wday++;

        }

        rtc_ds.clearAlarm (DS3232RTC::ALARM_2);

        rtc_ds.
        setAlarm ((hour != RTC_OMIT_HOUR ? DS3232RTC::ALM2_MATCH_HOURS :
                   DS3232RTC::ALM2_MATCH_MINUTES), wantedMinute, wantedHour,
                  (t.Wday % 7) + 1);

        rtc_ds.alarmInterrupt (DS3232RTC::ALARM_2, enabled);

    }

#endif

#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_PCF8563)
    {

        rtc_pcf.clearAlarm ();

        b = SmallRTC::_validateWakeup (wantedMinute, wantedHour, t, false);

        if (hour == RTC_OMIT_HOUR)
        {

            wantedHour = 99;

        }

        if (enabled)
        {

            rtc_pcf.setAlarm ((wantedMinute % 60), wantedHour, 99, 99);

        }
        else
        {

            rtc_pcf.resetAlarm ();

        }

    }

#endif

    if (m_rtc_pin && enabled)
    {

        esp_sleep_enable_ext0_wakeup ((gpio_num_t)m_rtc_pin, 0);

    }

}


uint8_t
SmallRTC::temperature ()
{

#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_DS3231)
    {

        return rtc_ds.temperature ();

    }

#endif

    return 255; //error

}


uint8_t
SmallRTC::getType ()
{

    return m_rtctype;

}


uint32_t
SmallRTC::getADCPin ()
{

    return m_adc_pin;

}


uint16_t
SmallRTC::getLocalYearOffset ()
{

    return 1900;

}


float
SmallRTC::getWatchyHWVer ()
{

    return f_watchyhwver;

}


void
SmallRTC::useESP32 (bool enforce, bool need32K)
{

#ifndef SMALL_RTC_NO_INT
    if (m_rtctype != RTC_ESP32)
    {

        b_forceesp32 = enforce;

    }

    if (need32K)
    {

        SmallRTC::use32K(need32K);

    }

#endif

}


bool
SmallRTC::onESP32 ()
{

    if (m_rtctype != RTC_ESP32)
    {

        return b_forceesp32;

    }
    return false;

}


time_t
SmallRTC::doMakeTime (tmElements_t tminput)
{

    tminput.Month++;

    tminput.Wday++;

    return makeTime (tminput);

}


void
SmallRTC::doBreakTime (time_t & T, tmElements_t & p_tminout)
{

    breakTime (T, p_tminout);

    p_tminout.Month--;

    p_tminout.Wday--;

}


bool
SmallRTC::isOperating ()
{

    return b_operational || m_rtctype == RTC_ESP32 || b_forceesp32;

}


void
SmallRTC::checkStatus (bool reset_op)
{

#ifndef SMALL_RTC_NO_DS3232
    if (b_operational && m_rtctype == RTC_DS3231)
    {

    b_operational = !rtc_ds.oscStopped (reset_op);

    }

#endif

}


float
SmallRTC::getRTCBattery (bool critical)
{

#ifndef SMALL_RTC_NO_DS3232
    if (m_rtctype == RTC_PCF8563)
    {

        return (critical ? 3.45 : 3.58);

    }

#endif

#ifndef SMALL_RTC_NO_PCF8563
    if (m_rtctype == RTC_DS3231)
    {

        return (critical ? 3.65 : 3.69);    // 3.69 : 3.75 (TEST)

    }
#endif

    return (critical ? 3.45 : 3.49);

}


void
SmallRTC::use32K (bool active)
{

    if (m_rtctype == RTC_ESP32 || b_forceesp32)
    {

        b_use32K = active;

    }

}


bool
SmallRTC::_validateWakeup (int8_t & mins, int8_t & hours,
                           tmElements_t & t_data, bool b_internal)
{

    int8_t tmp = 0;

    bool b_NoHour = false;

    bool b_nextDay = false;

    SmallRTC::read (t_data, b_internal);

    if (hours == -1)
    {

        hours = 0;
        b_NoHour = true;

    }

    hours &= 0x7F;

    tmp = (mins / 60);

    mins %= 60;

    if (tmp)
    {

        hours += tmp;

    }
    else if (mins < t_data.Minute && b_NoHour && !b_internal)
    {

        hours++;

    }

    b_nextDay |= (hours > 23 || hours > t_data.Hour);

    hours %= 24;

    if (b_internal)
    {

        mins -= t_data.Minute;

        if (mins < 0)
        {

            mins+=60;
            hours--;

        }

        if (!b_NoHour)
        {

            hours -= t_data.Hour;

            if (hours < 0)
            {

                hours+=24;

            }

        }

    }

    return b_nextDay;

}

String
SmallRTC::_getValue (String data, char separator, int index)
{

    int found = 0;

    int strIndex[] = { 0, -1 };

    int maxIndex = data.length () - 1;


for (int i = 0; i <= maxIndex && found <= index; i++)
    {

        if (data.charAt (i) == separator || i == maxIndex)
        {

            found++;

            strIndex[0] = strIndex[1] + 1;

            strIndex[1] = (i == maxIndex) ? i + 1 : i;

        }

    }

    return found > index ? data.substring (strIndex[0], strIndex[1]) : "";

}
