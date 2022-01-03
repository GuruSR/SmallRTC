#ifndef SMALL_RTC_H
#define SMALL_RTC_H

#include <Timelib.h>
#include <DS3232RTC.h>
#include <Rtc_Pcf8563.h>

#define UNKNOWN 9
#define DS3231 0
#define PCF8563 1
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
        uint8_t getType();
        uint32_t getADCPin();
        uint8_t temperature();
    private:
        String _getValue(String data, char separator, int index);
};

#endif
