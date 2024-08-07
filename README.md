# SmallRTC 2.4.3  [![Arduino Lint](https://github.com/GuruSR/SmallRTC/actions/workflows/main.yml/badge.svg)](https://github.com/GuruSR/SmallRTC/actions/workflows/main.yml)
A WatchyRTC replacement that offers more functionality, correct time.h and timelib.h operation and is NTP safe.

Function names changed in Version 2.3.5+, please be aware of them.

Functions showing [] around parameters, means optional.

Functions and their usage:

**init():**  Use this in the **switch (wakeup_reason)** in **default:**  Make it the first entry, so you can use the last function for Battery Voltage.  Now includes corrections for the DS3231 and includes detection of non-functioning RTC.

**setDateTime(String datetime):**  Originally from WatchyRTC.config(datetime), this is cleaned up and corrected, includes detection of non-functioning RTC.

**read(tmElements_t &tm):**  Use this to read the RTC's current time state in a tmElements_t variable.

**set(tmElements_t tm):**  Use this to set the tmElements_t variable contents into the RTC, typically can be from any source, most typically, SmallNTP (GuruSR).  This function also includes detection of non-functioning RTC.

**clearAlarm():**  Use this at any time you wake the Watchy except at reboot, do this in the **switch (wakeup_reason)** in **case ESP_SLEEP_WAKEUP_EXT0**.

**nextMinuteWake(bool Enabled = true):**  This should be in your `deepSleep()` function just in front of `esp_deep_sleep_start()`.  This functions offers a False (optional) that will not wake the watch up on the next minute, for those who wish to only enable buttons to wake.

**atMinuteWake(uint8_t Minute, bool Enabled = true):**
Use this instead of `nextMinuteWake`, as this will make the RTC wake up when the Minute data element matches the Minute you give it.  Just like `nextMinuteWake` it can use False (optional) here to also stop the wake up from happening.  atMinuteWake can accept up to 60 minutes of extra time on the Minute value, this allows for minute wraparound if you don't want to calculate it yourself.

**atTimeWake(uint8_t Hour, uint8_t Minute, bool Enabled = true):**
Use this function to request the RTC to wake up on the hour and minute.  The Minute can accept up to 120 minutes, these are rolled into hours and added to the Hour entry.  Any hour rollover is automatically accounted for.

**uint8_t temperature():** Imported from WatchyRTC for compatibility.

**uint8_t getType():**  Returns the rtcType as it is no longer exposed.

**uint32_t getADCPin():**  Returns the ADC_PIN, to see an example, see NOTE below. [^1] [^2]

**uint16_t getLocalYearOffset():**  Returns the Year Offset, hard coded to 1900.

**float getWatchyHWVer():**  Returns the detected Watchy hardware version #.

**void useESP32(bool Enforce, [bool need32K]):**  Tell SmallRTC to only use the Internal RTC.

**bool onESP32():**  Returns `true` if the Internal RTC is being used (by way of enforcement or hardware version).

**time_t doMakeTime(tmElements_t TM)** A TimeLib.h & time.h compliant version of `makeTime()`.

**doBreakTime(time_t &T, tmElements_t &TM)**  TimeLib.h & time.h compliant version of `breakTime()`.

**bool isOperating()** Returns `true` if the RTC is working properly.

**float getRTCBattery(bool Critical = false)** retrieves the low/critical battery voltages that will keep the RTC running properly.

**void beginDrift(tmElements_t &TM, [bool Internal]):**  Start the Drift Detection with the current time (TM) from a reliable source (time.is) and which RTC.

**void pauseDrift(bool Pause):**  Tells SmallRTC to stop drift alteration (useful when timers are running to avoid odd behavior).

**void endDrift(tmElements_t &TM, [bool Internal]):**  Complete Drift Detection and create Drift Value.  TM is the current time from the same source as `beginDrift`.

**uint32_t getDrift([bool Internal]):**  Returns the Drift Value in 100ths of a second.  This value is never negative.

**void setDrift(uint32_t Drift, bool isFast, [bool Internal]):**  Set the Drift Value, whether the RTC runs FAST and if setting the Internal RTC Drift Value or not.

**bool isFastDrift([bool Internal]):**  This returns whether the specified RTC's drift is Fast or not.

**bool isNewMinute():**  This will return `true` when a minute has actually passed.

**bool updatedDrift([bool Internal]):**  Returns `true` if time drift has happened since last time set on the specified RTC.

**bool checkingDrift([bool Internal]):**  Returns `true` if the specified RTC is currently doing a Drift Detection.

**void use32K(bool active):**  Tell SmallRTC for the Internal RTC to use the 32K timing.  Automatically on for Watchy V3.

[^2]:  Usage of the getADCPin() function:   `getBatteryVoltage() { return analogReadMilliVolts(RTC.getADCPin()) / 500.0f; }`  500.0f is a sample divider, please use the correct one for the supported version.

[^1]:  For the PCF8563, there are 2 variants, use the RTC.getADCPin() to determine where the UP Button is.

All `bool Internal` parameters are optional, though if you do not choose true, the useESP32 function's value is used.

**NOTE:**  As of 2.3.7, if any external RTC isn't recognized, the internal RTC will be forced on.

How to use in your Watchy:

`#include <SmallRTC.h>` <- instead of #include "WatchyRTC.h"

`...`

`SmallRTC RTC; // Declare RTC object` <- Instead of WatchyRTC RTC;

Use the functions as you need to, though I would recommend including SmallNTP and Olson2POSIX for a complete suite of functionality.

**NOTE:**  using dayStr or any other day function, will need to have Wday + 1, the week starts at Sunday with the Wday being the days SINCE Sunday as per time.h requirements.

tmElements_t from `read(tmElements &tm)` returns the following:

```
   tm.Second; /**< seconds after the minute - [ 0 to 59 ] */
   tm.Minute; /**< minutes after the hour - [ 0 to 59 ] */
   tm.Hour; /**< hours since midnight - [ 0 to 23 ] */
   tm.Day; /**< day of the month - [ 1 to 31 ] */
   tm.Wday; /**< days since Sunday - [ 0 to 6 ] */
   tm.Month; /**< months since January - [ 0 to 11 ] */
   tm.Year; /**< years since 1970 */
```

tmElements_t in use with `set(tmElements tm)` also expects the above values.

**Compilation:**

For those using PlatformIO, you may run into errors about settimeofday and gettimeofday, you're compiling with Linux includes, not Arduino.  To avoid the problem, you'll need to do the following:

In SmallRTC.h, change:

`#include <time.h>`
  
to 
  
`#include <%USERPROFILE%\.platformio\packages\toolchain-xtensa-esp32\xtensa-esp32-elf\include\sys\time.h>`  <- Windows  
`#include <~/.platformio/packages/toolchain-xtensa-esp32/xtensa-esp32-elf/include/sys/time.h>`  <- Linux

And compile.

For those not wanting the DS3232RTC or the PCF8563 or even the ESP32 Internal, you can now disable them via #defines.
Just be sure these are before your `#include <SmallRTC.h>`:

`#define SMALL_RTC_NO_DS3232`  
`#define SMALL_RTC_NO_PCF8563`  
`#define SMALL_RTC_NO_INT`

Remember, you need at least 1 present for the RTC code to do anything.

As of version 2.3.7, you do not need to set `esp_sleep_enable_ext0_wakeup` as it is now done when you use any of the RTCs that require it.
