#include "Arduino.h"
#include "clock.hpp"
#include "Wire.h"

// used for the get_alarm custom method
static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }

Clock::Clock() {

}

void Clock::begin() {
  if (!this->rtc.begin()) {
    this->current_time.hours = 0;
    this->current_time.minutes = 0;
    this->current_time.seconds = 0;
    this->clock_error = true;
  } else {
    if (this->rtc.lostPower()) {
      this->rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    this->update();
  }
  
  this->rtc.disable32K(); // 32K pin not in use
  this->rtc.writeSqwPinMode(DS3231_OFF); // DS3231_OFF disable signal
  this->rtc.clearAlarm(1);
  this->rtc.clearAlarm(2);
  this->rtc.disableAlarm(2); // only using alarm 1
  
}

void Clock::update() {
  DateTime now = this->rtc.now();

  this->current_time.hours = now.hour();
  this->current_time.minutes = now.minute();
  this->current_time.seconds = now.second();
}

/*
 * This is basically a port line-per-line from the "now()" function in the RTClib BUT
 * accessing the ALARM1 first address (0x07) instead of the regular clock one (0x00)
 * (this works only on the DS3231 and it's here because I think the RTClib cannot read the
 * already set data from those alarm registers)
 * https://github.com/adafruit/RTClib/blob/master/RTClib.cpp
 * https://github.com/adafruit/RTClib/blob/master/RTClib.h
 * https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
 */
Clock_time Clock::get_alarm() {
  Clock_time alarm;
    
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write((byte) DS3231_ALARM1); // request the data from DS3231_ALARM1
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 3); // fetch the first 3 relevant values: hours, minutes and seconds
  // 0x7F to remove the high bits since we doesn't need them (same with minutes, but not present in the original code
  // (not necessary as far as I've read but I'll preserve the original function method)
  Wire.read(); // just read and discard the seconds (doesn't use it, otherwise bcd2bin(Wire.read() & 0x7F))
  alarm.seconds = 0;
  alarm.minutes = bcd2bin(Wire.read());
  alarm.hours = bcd2bin(Wire.read());
  
  return alarm;
}

Clock_time Clock::get_time() {
  return this->current_time;
}

String Clock::get_time_str() {
  return String("hours: " + String(this->current_time.hours) + 
         ", minutes: " + String(this->current_time.minutes) +
         ", seconds: " + String(this->current_time.seconds));
}

bool Clock::get_clock_error() {
  return this->clock_error;
}

// return true if alarm was fired and clear if proceed (so it will be triggered only ONE time)
bool Clock::get_alarm_fired() {
  if(rtc.alarmFired(1)) {
    rtc.clearAlarm(1);
    return true;
  }
  return false;
}

bool Clock::set_alarm(Clock_time new_alarm) {
  // match only hour + minutes + seconds (nevermind the day/month/year stuff)
  return this->rtc.setAlarm1(DateTime(2000, 1, 1, new_alarm.hours, new_alarm.minutes, 0), DS3231_A1_Hour); // use DS3231_A1_Second instead of DS3231_A1_Hour to debug every minute
}

void Clock::set_time(Clock_time new_time) {
  // always reset to 2000/1/1
  this->rtc.adjust(DateTime(2000, 1, 1, new_time.hours, new_time.minutes, 0));
}
