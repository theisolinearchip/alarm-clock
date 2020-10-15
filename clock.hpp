#include "RTClib.h"

#ifndef Clock_h
#define Clock_h

struct Clock_time {
  int hours;
  int minutes;
  int seconds;
};

class Clock {
  public:
    
    Clock::Clock();

    void begin();
    void update();
    Clock_time get_alarm();
    Clock_time get_time();
    String get_time_str();
    bool get_clock_error();
    bool get_alarm_fired();
    bool set_alarm(Clock_time new_alarm);
    void set_time(Clock_time new_time);
    
  private:

    RTC_DS3231 rtc;
    Clock_time current_time;
    
    bool clock_error;
    
};

#endif
