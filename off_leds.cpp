#include "Arduino.h"
#include "off_leds.hpp"

Off_leds::Off_leds(int off_leds_size, int *off_leds_pins) {
  this->off_leds_size = off_leds_size;
  this->off_leds_pins = off_leds_pins;

  for (int i = 0; i < this->off_leds_size; i++) {
    pinMode(this->off_leds_pins[i], OUTPUT);
    digitalWrite(this->off_leds_pins[i], LOW);
  }
}

// level - 1 'cause start at level 0 = no leds
void Off_leds::set_off_led_level(int level) {
  for (int i = 0; i < this->off_leds_size; i++) {
    digitalWrite(this->off_leds_pins[i], (i <= level-1) ? HIGH : LOW);
  }
}
