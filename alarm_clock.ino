#include <Adafruit_LEDBackpack.h>
#include <RTClib.h>
#include "two_keys.hpp"
#include "keypad.hpp"
#include "off_leds.hpp"
#include "clock.hpp"

// PINS

// two-keys
const int left_key_pin = 5;
const int left_key_led_pin = 3;

const int right_key_pin = 4;
const int right_key_led_pin = 2;

// keyboard
// left to right pins on the keypad
// c2, r1, c1, r4, c3, r3, r2
// if starting at 13 and decrementing (from 13 to 6) then, in order:
// c2-13, r1-12, c1-11, r4-10, c3-9, r3-8, r2-7
byte keypad_row_pins[Keypad::ROWS_BYTE] = {12, 7, 8, 10};
byte keypad_col_pins[Keypad::COLUMNS_BYTE] = {11, 13, 9};
const int keypad_code_size = 4;
const int keypad_led_pins[keypad_code_size] = {42, 43, 44, 45};

// off button and leds
const int alarm_off_button_pin = 36;
const int off_leds_size = 2;
const int off_leds_pins[off_leds_size] = {30, 28};

const int alarm_pin = 52;

// MODES

static const int MODE_CLOCK = 1;
static const int MODE_CONFIG = 2;
static const int MODE_ALARM_BEEPING = 3;

static const int ALARM_IDLE = 0;
static const int ALARM_FULL_ARMED = 1; // aka "waiting for the keys sequence"
static const int ALARM_WAITING_CODE = 2;
static const int ALARM_WAITING_BUTTON = 3;

// CONFIG CONSTANTS
static const unsigned int half_second_millis = 500;
static const unsigned int second_millis = 1000;

// CLOCK CONTROL VARIABLES
int mode = MODE_CLOCK;
int alarm_mode = ALARM_IDLE;
bool alarm_active = false;
bool hour_am_pm = true; // if false, 24 hour mode
bool config_clock_or_alarm = true; // if false, we're setting the alarm
Clock_time current_clock; // get at the beginning of each loop, will be updated when finishing the clock config with the new value
Clock_time config_clock; // snapshot of current minute/hour from CLOCK to be changed when entering the config mode
Clock_time config_alarm; // snapshot of current minute/hour from ALARM to be changed when entering the config mode

// MODULES
Adafruit_7segment clock_display = Adafruit_7segment();
Two_keys two_keys = Two_keys(left_key_pin, left_key_led_pin, right_key_pin, right_key_led_pin);
Keypad keypad = Keypad(keypad_row_pins, keypad_col_pins, keypad_code_size, keypad_led_pins);
Off_leds off_leds = Off_leds(off_leds_size, off_leds_pins);
Clock clock = Clock();

unsigned long current_millis; // since they're unsigned they're overflow safe
unsigned long previous_millis;
unsigned long elapsed_millis;
unsigned long current_half_second_millis; // used to trigger some updates ONLY each HALF SECOND (like clock);
unsigned long current_second_millis; // used to trigger some updates ONLY each SECOND (like the : on the display);

void setup() {
  
  // debug
  // Serial.begin(9600);

  current_millis = millis();
  previous_millis = current_millis;
  elapsed_millis = 0;
  current_half_second_millis = 0;

  clock_display.begin(0x70);
  keypad.begin();
  clock.begin();

  pinMode(alarm_off_button_pin, INPUT_PULLUP);
  pinMode(alarm_pin, OUTPUT);
  digitalWrite(alarm_pin, LOW);
  
  change_mode(MODE_CLOCK);

}

void loop() {

  // check for clock errors (right now the error is set to true if
  // we cannot connect to the clock module via I2C at the beginning)
  if (clock.get_clock_error()) {
    clock_display.printError();
    clock_display.writeDisplay();
    return;
  }

  current_millis = millis();
  elapsed_millis = current_millis - previous_millis;
  current_half_second_millis += elapsed_millis;
  current_second_millis += elapsed_millis;

  // always check to clean the result but process
  // only when MODE_CLOCK (that means the other modes
  // will "skip" the alarm and not ignore it until
  // the clock mode checks for it when returning to it)
  bool alarm_fired = clock.get_alarm_fired();

  switch (mode) {
    default:
    case MODE_CLOCK:
      if (alarm_fired && alarm_active) {
        change_mode(MODE_ALARM_BEEPING);
      }
      
      if (keypad.is_config_mode_requested()) {
        change_mode(MODE_CONFIG);
      } else if (keypad.is_alarm_toggle_requested()) {
        toggle_alarm();
      } else if (keypad.is_hour_format_toggle_requested()) {
        toggle_hour_format();
      }
      break;
    case MODE_CONFIG: {
      if (process_config()) {
        change_mode(MODE_CLOCK);
      }
      break;
    }
    case MODE_ALARM_BEEPING:
      // beep
      if (process_alarm_states()) {
        change_mode(MODE_CLOCK);
      }
      break;
  }

  // time and update stuff
  previous_millis = current_millis;

  if (current_half_second_millis >= half_second_millis) {
    current_half_second_millis = 0;
  }

  if (current_second_millis >= second_millis) {
    current_second_millis = 0;
  }

  two_keys.update(elapsed_millis);
  keypad.update(elapsed_millis);
  clock.update();
  current_clock = clock.get_time();
  
  // display
  set_display();

}

// some modes requires some inits when changing to it, use always this function to do that
void change_mode(int new_mode) {
  switch (new_mode) {
    case MODE_CLOCK:
      digitalWrite(alarm_pin, LOW);
      change_alarm_mode(ALARM_IDLE);
      break;
    case MODE_CONFIG: {
      digitalWrite(alarm_pin, LOW);
      change_alarm_mode(ALARM_IDLE);
      // after setting the alarm, change the keypad to config mode
      keypad.set_config_mode();
      config_clock_or_alarm = true; // always start setting the clock
      hour_am_pm = false; // always change to 24 hours
      config_clock = current_clock;
      config_alarm = clock.get_alarm();
      break;
    }
    case MODE_ALARM_BEEPING:
      digitalWrite(alarm_pin, HIGH);
      change_alarm_mode(ALARM_FULL_ARMED);
      break;
  }

  mode = new_mode;
}

void change_alarm_mode(int new_alarm_mode) {
  switch (new_alarm_mode) {
    default:
    case ALARM_IDLE:
      two_keys.idle();
      keypad.idle();
      off_leds.set_off_led_level(0);
      break;
    case ALARM_FULL_ARMED:
      two_keys.init();
      keypad.idle();
      off_leds.set_off_led_level(0);
      break;
    case ALARM_WAITING_CODE:
      keypad.init();
      off_leds.set_off_led_level(1);
      break;
    case ALARM_WAITING_BUTTON:
      off_leds.set_off_led_level(2);
      break;
  }

  alarm_mode = new_alarm_mode;
}

// will return true if alarm ends
bool process_alarm_states() {
  
  bool alarm_finished = false;
  int alarm_off_button_status = digitalRead(alarm_off_button_pin); // reset if off button is pushed BEFORE the sequence
  switch (alarm_mode) {
    default:
    case ALARM_IDLE:
      break;
    case ALARM_FULL_ARMED:
      if (two_keys.is_final_valid_status()) {
        change_alarm_mode(ALARM_WAITING_CODE);
      } else if (alarm_off_button_status == LOW) {
        change_alarm_mode(ALARM_FULL_ARMED);
      }
      break;
    case ALARM_WAITING_CODE:
      if (keypad.is_final_valid_status()) {
        change_alarm_mode(ALARM_WAITING_BUTTON);
      } else if (alarm_off_button_status == LOW) {
        change_alarm_mode(ALARM_FULL_ARMED);
      }
      break;
    case ALARM_WAITING_BUTTON:
      alarm_finished = (alarm_off_button_status == LOW);
      break;
  }

  return alarm_finished;
}

void toggle_alarm() {
  alarm_active = !alarm_active;
}

void toggle_hour_format() {
  hour_am_pm = !hour_am_pm;
}

// will return true if config ends (despite saving or not the info)
bool process_config() {
  bool config_finished = false;
  
  int key = keypad.get_config_mode_key();
  if (key != Keypad::NO_KEY) {
    switch (key) {
      case Keypad::CONFIG_KEY_EXIT_NO_SAVE:
        change_mode(MODE_CLOCK);
        break;
      case Keypad::CONFIG_KEY_EXIT_SAVE:
        // save ONLY the mode we're configuring
        if (config_clock_or_alarm) {
          clock.set_time(config_clock);
        } else {
          clock.set_alarm(config_alarm);
        }

        // update the current_clock to be displayed with the new one (will be updated on the next loop)
        current_clock = config_clock;
        change_mode(MODE_CLOCK);

        break;
      case Keypad::CONFIG_KEY_TOGGLE_CLOCK_ALARM:
        // toggle alarm / clock setting
        config_clock_or_alarm = !config_clock_or_alarm;
        break;
      default: {// +1/-1 hour / minute
        int hour_inc = 0;
        int minute_inc = 0;
        if (key == Keypad::CONFIG_KEY_INC_HOURS) hour_inc++;
        else if (key == Keypad::CONFIG_KEY_DEC_HOURS) hour_inc--;
        else if (key == Keypad::CONFIG_KEY_INC_MIN) minute_inc++;
        else if (key == Keypad::CONFIG_KEY_DEC_MIN) minute_inc--;
        else break;

        if (config_clock_or_alarm) {
          config_clock.hours += hour_inc;
          config_clock.minutes += minute_inc;
          if (config_clock.hours > 23) config_clock.hours = 0;
          else if (config_clock.hours < 0) config_clock.hours = 23;
          else if (config_clock.minutes > 59) config_clock.minutes = 0;
          else if (config_clock.minutes < 0) config_clock.minutes = 59;
        } else {
          config_alarm.hours += hour_inc;
          config_alarm.minutes += minute_inc;
          if (config_alarm.hours > 23) config_alarm.hours = 0;
          else if (config_alarm.hours < 0) config_alarm.hours = 23;
          else if (config_alarm.minutes > 59) config_alarm.minutes = 0;
          else if (config_alarm.minutes < 0) config_alarm.minutes = 59;
        }

        // notice that we're not using seconds nor any other info
        break;
      }
    }
  }

  return config_finished;
}

void set_display() {
  
  int clock_value;
  switch(mode) {
    case MODE_CLOCK:
    case MODE_ALARM_BEEPING: {

      // alarm and clock works the same, the only difference is the
      // delay for the : symbol while blinking
      clock_value = (current_clock.hours - ((hour_am_pm && current_clock.hours > 12) ? 12 : 0)) * 100 + current_clock.minutes;

      if (mode == MODE_ALARM_BEEPING) {
        clock_display.drawColon(current_half_second_millis < half_second_millis/2);
      } else { // CLOCK
        clock_display.drawColon(current_second_millis < second_millis/2);
      }
      
      // AM/PM dot only if proceed (XX.:YY if XX is hour > 12, otherwise X.X:YY; if 24h mode then XX:YY)
      
      clock_display.writeDigitNum(0, clock_value%10000/1000, (hour_am_pm && current_clock.hours <= 12));
      clock_display.writeDigitNum(1, clock_value%1000/100, (hour_am_pm && current_clock.hours > 12));
      clock_display.writeDigitNum(3, clock_value%100/10, false);

      // show dot if alarm activated (do not show in config mode since it's not relevant and ignored)
      clock_display.writeDigitNum(4, clock_value%10, alarm_active);

      break;
    }
    case MODE_CONFIG: {
      // show tmp clock / alarm based on config_clock_or_alarm and other flags

      if (config_clock_or_alarm) {
        // setting up the clock
        clock_value = config_clock.hours * 100 + config_clock.minutes; // display 24 hours always
      } else {
        // alarm
        clock_value = config_alarm.hours * 100 + config_alarm.minutes; // display 24 hours always
      }

      clock_display.writeDigitNum(0, clock_value%10000/1000, config_clock_or_alarm);
      clock_display.writeDigitNum(1, clock_value%1000/100, config_clock_or_alarm);
      clock_display.writeDigitNum(3, clock_value%100/10, !config_clock_or_alarm);
      clock_display.writeDigitNum(4, clock_value%10, !config_clock_or_alarm);
      clock_display.drawColon(true);
      
      break;
    }
  }

  clock_display.writeDisplay();
  
}
