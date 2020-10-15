#include "Arduino.h"
#include "keypad.hpp"

Keypad::Keypad(byte *row_pins, byte *col_pins, int code_size, int *led_pins) :
  keypad(makeKeymap(Keypad::KEYS), row_pins, col_pins, Keypad::ROWS, Keypad::COLUMNS)
  // init class inside class: https://stackoverflow.com/questions/45356776/initializing-class-inside-class
{
  this->code_size = code_size;
  this->led_pins = led_pins;

  for (int i = 0; i < this->code_size; i++) {
    pinMode(this->led_pins[i], OUTPUT);
    digitalWrite(this->led_pins[i], LOW);
  }
}

void Keypad::begin() {
  this->keypad.begin();
}

void Keypad::init() {
  this->change_mode(Keypad::MODE_WAITING_CODE);
}

void Keypad::idle() {
  this->change_mode(Keypad::MODE_IDLE);
}

bool Keypad::is_final_valid_status() {
  return (this->mode == Keypad::MODE_CODE_VALID);
}

bool Keypad::is_config_mode_requested() {
  return this->config_mode_requested;
}

void Keypad::set_config_mode() {
  this->change_mode(Keypad::MODE_CONFIG);
}

int Keypad::get_config_mode_key() {
  return this->current_config_key_pressed;
}

bool Keypad::is_alarm_toggle_requested() {
  return this->alarm_toggle_requested;
}

bool Keypad::is_hour_format_toggle_requested() {
  return this->hour_format_toggle_requested;
}

void Keypad::update(unsigned long current_accumulated_millis) {
  this->keypad.tick();

  // get key for this tick
  int key = Keypad::NO_KEY;
  int event = Keypad::NO_EVENT;
  bool is_long_click = false;
  if (this->keypad.available()) {
    keypadEvent e = this->keypad.read();
    event = e.bit.EVENT;
    key = (char)e.bit.KEY - '0';
  }

  if (this->current_key_pressed != Keypad::NO_KEY) { // a key was already pressed
    if (event == KEY_JUST_RELEASED) { // release everything (don't check for multiples keys, next tick will act as a new one)
      this->current_key_pressed = Keypad::NO_KEY;
      this->current_long_click_millis = 0;
    } else if (event == Keypad::NO_EVENT) {
      // increment long click time
      this->current_long_click_millis += current_accumulated_millis;
      if (this->current_long_click_millis >= Keypad::MILLIS_LONG_CLICK_WAIT) {
        is_long_click = true;
        this->current_long_click_millis = 0; // yep, that will allow multiple long clicks, one each MILLIS_LONG_CLICK_WAIT, without releasing
      }
    }
  } else { // new key pressed;
    this->current_key_pressed = key;
    this->current_long_click_millis = 0;
  }

  this->current_config_key_pressed = Keypad::NO_KEY;

  switch (this->mode) {
    case Keypad::MODE_IDLE: // change to config mode can be requested only from idle mode
      this->config_mode_requested = (is_long_click && this->current_key_pressed == Keypad::ASTERISC_ASCII);
      this->alarm_toggle_requested = (is_long_click && this->current_key_pressed == Keypad::HASHTAG_ASCII);
      this->hour_format_toggle_requested = (is_long_click && this->current_key_pressed == 0);
      break;
    case Keypad::MODE_WAITING_CODE: {
        // waiting code won't handle any long click
        if (event == KEY_JUST_PRESSED && this->current_key_pressed != Keypad::NO_KEY) {
          if (this->current_code_digit < this->code_size) {
            // regular input for the code

            this->current_code[this->current_code_digit] = this->current_key_pressed;
            this->current_code_digit++;

            this->set_led_level(this->current_code_digit);
          } else if (this->current_key_pressed == Keypad::HASHTAG_ASCII) {
            // confirm with #

            this->current_code_digit = 0; // safety first

            bool valid = true;
            for (int i = 0; i < this->code_size; i++) {
              valid = valid && (this->current_code[i] == Keypad::VALID_CODE[i]);
            }

            if (valid) {
              this->change_mode(Keypad::MODE_CODE_VALID);
            } else {
              this->change_mode(Keypad::MODE_WAITING_CODE);
            }
          } // else and reset if the 5th digit is not a HASHTAG?
        }

        break;
      }
    case Keypad::MODE_CODE_VALID:
      break;
    case Keypad::MODE_CONFIG:
      if (event == KEY_JUST_PRESSED && this->current_key_pressed != Keypad::NO_KEY) {
        this->current_config_key_pressed = this->current_key_pressed;
      }
      // won't exit from mode_config from here, clock will handle this
      break;
  }
}

void Keypad::change_mode(int new_mode) {
  this->config_mode_requested = false;
  this->alarm_toggle_requested = false;
  this->hour_format_toggle_requested = false;

  switch (new_mode) {
    case Keypad::MODE_IDLE:
      this->accumulated_millis = 0;
      this->current_long_click_millis = 0;
      this->set_led_level(0);
      break;
    case Keypad::MODE_WAITING_CODE: {
        for (int i = 0; i < this->code_size; i++) {
          this->current_code[i] = -1;
        }
        this->current_code_digit = 0;
        this->set_led_level(0);
        break;
      }
    case Keypad::MODE_CODE_VALID:
      this->set_led_level(this->code_size);
      break;
    case Keypad::MODE_CONFIG:
      break;
  }

  this->mode = new_mode;
}

// level - 1 'cause start at level 0 = no leds
void Keypad::set_led_level(int level) {
  for (int i = 0; i < this->code_size; i++) {
    digitalWrite(this->led_pins[i], (i <= level - 1) ? HIGH : LOW);
  }
}
