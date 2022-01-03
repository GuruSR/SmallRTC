# SmallRTC
A WatchyRTC replacement that offers more functionality, correct time.h and timelib.h operation and is NTP safe.

Functions and their usage:

**init():**  Use this in the **switch (wakeup_reason)** in **default:**  Make it the first entry, so you can use the last function for Battery Voltage.

**setDateTime(String datetime):**  Originally from WatchyRTC.config(datetime), this is cleaned up and corrected.

**read(tmElements_t &tm):**  Use this to read the RTC's current time state in a tmElements_t variable.

**set(tmElements_t tm):**  Use this to set the tmElements_t variable contents into the RTC, typically can be from any source, most typically, SmallNTP (GuruSR).

**resetWake():**  This resets the Interrupt that woke the ESP32 up, do this in the **switch (wakeup_reason)** in **case ESP_SLEEP_WAKEUP_EXT0**.

**nextMinuteWake(bool Enabled = true):**  This should be in your deepSleep() function just in front of esp_deep_sleep_start().  This functions offers a False that will not wake the watch up on the next minute, for those who wish to only enable buttons to wake.

**uint8_t temperature():** Imported from WatchyRTC for compatibility.

**uint8_t getType():**  Returns the rtcType as it is no longer exposed.

**uint32_t getADCPin():**  Returns the ADC_PIN, so your BatteryVoltage function can look like:

`getBatteryVoltage() { return analogReadMilliVolts(RTC.getADCPin()) / 500.0f; }`

How to use in your Watchy.

`#include <SmallRTC.h>`

`...`

`SmallRTC RTC; // Declare RTC object`

Use the functions as you need to, though I would recommend including SmallNTP and Olson2POSIX for a complete suite of functionality.
