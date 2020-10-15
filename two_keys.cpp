#include "Arduino.h"
#include "two_keys.hpp"

Two_keys::Two_keys(int left_key_pin, int left_key_led_pin, int right_key_pin, int right_key_led_pin) {
  this->left_key_pin = left_key_pin;
  this->left_key_led_pin = left_key_led_pin;
  
  this->right_key_pin = right_key_pin;
  this->right_key_led_pin = right_key_led_pin;

  pinMode(this->left_key_pin, INPUT_PULLUP);
  pinMode(this->right_key_pin, INPUT_PULLUP);
  
  pinMode(this->left_key_led_pin, OUTPUT);
  pinMode(this->right_key_led_pin, OUTPUT);

  // this->init();
}

// call it every time we start checking for keys stuff
void Two_keys::init() {

  // go to reset if, when init, some key is turned on
  if (digitalRead(this->left_key_pin) == LOW || digitalRead(this->right_key_pin) == LOW) {
    this->change_mode(Two_keys::MODE_RESET);
  } else {
    this->change_mode(Two_keys::MODE_OFF);
  }
    
}

void Two_keys::idle() {
  this->change_mode(Two_keys::MODE_IDLE);
}

bool Two_keys::is_final_valid_status() {
  return (this->mode == Two_keys::MODE_TWO);
}

void Two_keys::update(unsigned long current_accumulated_millis) {

  switch(this->mode) {
    case Two_keys::MODE_RESET:
      // add accumulated millis
      // if > MILLIS_RESET_INTERVAL change_mode to MODE_OFF
      
      if (digitalRead(this->left_key_pin) == LOW || digitalRead(this->right_key_pin) == LOW) {
        // start counting once both of the keys are off
        this->accumulated_millis = 0;
      } else {      
        this->accumulated_millis += current_accumulated_millis;
        if (this->accumulated_millis >= Two_keys::MILLIS_RESET_INTERVAL) {
          this->change_mode(Two_keys::MODE_OFF);
        }
      }
      
      break;
    default:
    case Two_keys::MODE_IDLE:
      // nothing to do on idle
      break;
    case Two_keys::MODE_OFF:
      // check for changes on the switches
      // trigger change_mode to MODE_ONE if proceed

      if (digitalRead(this->left_key_pin) == LOW) {
        digitalWrite(this->left_key_led_pin, HIGH);
        this->change_mode(Two_keys::MODE_ONE);
      } else if (digitalRead(this->right_key_pin) == LOW) {
        digitalWrite(this->right_key_led_pin, HIGH);
        this->change_mode(Two_keys::MODE_ONE);
      }
      
      break;
    case Two_keys::MODE_ONE: { // define scope with { } since we're declaring local vars inside: https://stackoverflow.com/questions/92396/why-cant-variables-be-declared-in-a-switch-statement
      
      int left_key = digitalRead(this->left_key_pin);
      int right_key = digitalRead(this->right_key_pin);

      // add accumulated millis
      // if > MILLIS_KEYS_INTERVAL change to RESET
      // check for BOTH keys, if proceed, change to MODE_TWO
      // notice that if there's NOTHING, change to MODE_OFF

      this->accumulated_millis += current_accumulated_millis;
      if (this->accumulated_millis >= Two_keys::MILLIS_KEYS_INTERVAL) {
        this->change_mode(Two_keys::MODE_RESET);
      } else if (left_key == HIGH && right_key == HIGH) {
        this->change_mode(Two_keys::MODE_OFF);
      } else if (left_key == LOW && right_key == LOW) {
        this->change_mode(Two_keys::MODE_TWO);
      }
      break;
    }
    
    case Two_keys::MODE_TWO:
      // actually do nothing
      break;
     
  }

}

void Two_keys::change_mode(int new_mode) {

  switch(new_mode) {
    case Two_keys::MODE_RESET:
      this->accumulated_millis = 0;
      digitalWrite(this->left_key_led_pin, LOW);
      digitalWrite(this->right_key_led_pin, LOW);
      break;
    default:
    case Two_keys::MODE_IDLE:
      digitalWrite(this->left_key_led_pin, LOW);
      digitalWrite(this->right_key_led_pin, LOW);
      break;
    case Two_keys::MODE_OFF:
      digitalWrite(this->left_key_led_pin, LOW);
      digitalWrite(this->right_key_led_pin, LOW);
      break;
    case Two_keys::MODE_ONE:
      this->accumulated_millis = 0;
      break;
    case Two_keys::MODE_TWO:
      digitalWrite(this->left_key_led_pin, HIGH);
      digitalWrite(this->right_key_led_pin, HIGH);
      break;
  }

  this->mode = new_mode;
  
}
