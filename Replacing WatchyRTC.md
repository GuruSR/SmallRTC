First off, WatchyRTC and SmallRTC have a similar build, they both use tmElements_t for setting/reading the time, though this is where the similarities end.

The init for WatchyRTC is done on every boot of the Watchy, SmallRTC's init is done ONLY at the first boot after a reset.

So remove the RTC.init() from where you see it in Watchy.cpp and relocate it to in the switch default:

(-> arrows point to changes <-)
```
void Watchy::init(String datetime) {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause(); // get wake up reason
  Wire.begin(SDA, SCL);                         // init i2c
->  RTC.init(); <- move this down to the default: // reset below just above the other RTC call.

  // Init the display here for all cases, if unused, it will do nothing
  display.epd2.selectSPI(SPI, SPISettings(20000000, MSBFIRST, SPI_MODE0)); // Set SPI to 20Mhz (default is 4Mhz)
  display.init(0, displayFullInit, 10,
               true); // 10ms by spec, and fast pulldown reset
  display.epd2.setBusyCallback(displayBusyCallback);

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
    RTC.read(currentTime);
->  RTC.clearAlarm(); <-  Add this here.
    switch (guiState) {
    case WATCHFACE_STATE:
      showWatchFace(true); // partial updates on tick
      if (settings.vibrateOClock) {
        if (currentTime.Minute == 0) {
          // The RTC wakes us up once per minute
          vibMotor(75, 4);
        }
      }
      break;
    case MAIN_MENU_STATE:
      // Return to watchface if in menu for more than one tick
      if (alreadyInMenu) {
        guiState = WATCHFACE_STATE;
        showWatchFace(false);
      } else {
        alreadyInMenu = true;
      }
      break;
    }
    break;
  case ESP_SLEEP_WAKEUP_EXT1: // button Press
    handleButtonPress();
    break;
  default: // reset
-> RTC.init(); <- This is where this belongs.
-> RTC.setDateTime(datetime); <- WatchyRTC original RTC.config(datetime);
```

The RTC.config from WatchyRTC becomes setDateTime in SmallRTC, though both use the same data.

The deepSleep function also needs changes:

```
void Watchy::deepSleep() {
  display.hibernate();
  if (displayFullInit) // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  displayFullInit = false; // Notify not to init it again
-> RTC.nextMinuteWake(); <- This can either be nextMinuteWake OR atMinuteWake(minute), both offer optional Enabled.

  // Set GPIOs 0-39 to input to avoid power leaking out
  const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
  for (int i = 0; i < GPIO_NUM_MAX; i++) {
    if ((ignore >> i) & 0b1)
      continue;
    pinMode(i, INPUT);
  }
-> if (!RTC.OnESP32()) esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN,
                               0); // enable deep sleep wake on RTC interrupt <- Hardware Version 3 apparently has no external RTC
  esp_sleep_enable_ext1_wakeup(
      BTN_PIN_MASK,
      ESP_EXT1_WAKEUP_ANY_HIGH); // enable deep sleep wake on button press
  esp_deep_sleep_start();
}
```

The RTC.nextMinueWake and RTC.atMinuteWake both work the same, nextMinuteWake takes the current minute and increases it (within 0 to 59) and the it calls atMinuteWake.  Neither use the DS3231M's "next minute" wake up, but rather the "on the specified minute" wake up.

The RTC.OnESP32() will be TRUE if you've forced the use of the ESP32's internal RTC or the Hardware Version is 3.0, which SmallRTC (2.3 and above) will detect.  (As of SmallRTC 2.3, it does not detect the ADC pin for Version 3.0 as that information isn't available.)  The reason the RTC.OnESP32() if statement needs to be added, is to avoid the external RTC from causing a second wakeup when you're not using it.

The drift detection in SmallRTC (2.3 and above) is an approximation due to the 2 decimal point restriction on math functions, which may not be accurate on either internal or external RTCs.  Be mindful that the RTCs will drift depending on voltage level and temperature unless internally designed to compensate.
