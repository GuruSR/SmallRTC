# **Drift Calculation**

For those wanting to take advantage of Drift Management on your RTC(s), you should read this fully and return to it if you have any issues.

Drift Calculation does the following:

1. Start with the current time.  `beginDrift(time from reliable source, Internal)`
2. The user should wait 1 day (or more, more is okay but not necessarily better) before doing 3.
3. End the drift calculation with the current time.  `endDrift(time from reliable source, Internal)`
4. The difference in time is calculated over the distance traveled (start to time that it got to).
5. Prepare the result * 100 and floored to strip off any excess.

## **Drift Variance**

This is when the RTC is in a different environment during your daily usage, this can cause the internal RTC (and some externals) to drift based on the changing environment.  No drift calculation that is software based can actually detect this over a large period without keeping the RTC and the hardware always on, even then NTP would need to be done hourly to monitor changes.

## **Drift Management**

SmallRTC 2.3 (and above) has the ability to on-the-fly change the Drift value, allowing external code to give the user the ability to alter it until it gets as close to possible for time as they can.  The Drift Calculation is accurate (with a variance of error due to the 2 decimal float math), but it was accurate for the conditions the RTC was in at that time and will undoubtably be wrong in others.

Your best solution is to find a balance to keep it from getting to fast or too slow that it isn't reliable enough to work with.  Watchy_GSR has the option of doing an NTP on a specific time of day (so you could set it to go an hour before you'd usually get up), this would give the RTC the chance to be mostly accurate all day.  Also as of 1.4.7C, Watchy_GSR has the option to alter the Drift value so you can raise (slow down) or lower (speed up) second changes.

### **Drift Value**

The Drift Value is an unsigned 32 bit integer that determines the distance in time between updates.  The Drift Value when created, is in 100ths of a second, to allow the storage of the value in an unsigned 32bit integer, the real value is Drift Value / 100.  The excess partial second is stored in a slush, where it is added onto until it becomes a whole number, that whole number is passed onto the last changed time, to give the Drift Value for the next change a chance to wait 1 second longer.  Obviously this may/may not be as accurate as wanted, since you can't split seconds on most the RTCs supported, nor would you want to wake the cpu up each second to check if time needs to be corrected.  Keeping all of this in mind will explain why the calculation and the actual RTC drift will vary.

### **Editing the Drift Value**

After the Drift Calculation is completed and your RTC was originally going FAST:

- Lowering the Drift Value will cause it to drift SLOWER, as raising the Drift Value will cause it to drift FASTER.

After the Drift Calculation is completed and your RTC was originally going SLOW:

- Lowering the Drift Value will cause it to drift FASTER, as raising the Drift Value will cause it to drift SLOWER.

### **Coding Notes**

- `read(tmElements_t)` is where Drift Management happens, so upon wakeup, check the **Second** value, if at 59, you may want to loop wait until it hits the new minute as a fast drifiting RTC will encounter this and can cause grief with alarms.
- It is up to the software using this to record the Drift Value for either/both RTC(s) and restore them at some point during initial reboot.
- The Drift Value can be changed at any time and will not affect the existing Slush storage on either Internal or External RTC.
- `set(tmElements_t)` does reset the Slush value on the RTC being used, if Internal is being used/forced, it will not reset the External RTC, if the External RTC is used, both Internal and External will be set to the current time and their Slushs zeroed.
