#include "Adafruit_Keypad.h"

#ifndef Keypad_h
#define Keypad_h

class Keypad {
  public:
    
    static const int ROWS = 4;
    static const int COLUMNS = 3;

    static const byte ROWS_BYTE = 4; 
    static const byte COLUMNS_BYTE = 3;
    
    //constructor
    Keypad::Keypad(byte *row_pins, byte *col_pins, int code_size, int *led_pins);
    
    void begin();
    void init();
    void idle();
    bool is_final_valid_status();
    bool is_config_mode_requested();
    void set_config_mode();
    int get_config_mode_key();
    bool is_alarm_toggle_requested();
    bool is_hour_format_toggle_requested();
    void update(unsigned long current_accumulated_millis);

    // keys
    static const int NO_KEY = -1;
    static const int ASTERISC_ASCII = -6;
    static const int HASHTAG_ASCII = -13;

    static const int CONFIG_KEY_EXIT_NO_SAVE = 0;
    static const int CONFIG_KEY_EXIT_SAVE = Keypad::HASHTAG_ASCII;
    static const int CONFIG_KEY_TOGGLE_CLOCK_ALARM = 2;
    static const int CONFIG_KEY_INC_HOURS = 1;
    static const int CONFIG_KEY_DEC_HOURS = 4;
    static const int CONFIG_KEY_INC_MIN = 3;
    static const int CONFIG_KEY_DEC_MIN = 6;
    
  private:

    Adafruit_Keypad keypad;
    int code_size;
    int *led_pins;
    
    static const unsigned long MILLIS_LONG_CLICK_WAIT = 1000; // time holding a button to consider it "a long click"
    
    static const int MODE_IDLE = 0; // nothing happens, but can read long press to change to alarm on/off or clock/alarm hour setting
    static const int MODE_WAITING_CODE = 1; // waiting for a 4-digit code (to use when alarm is beeping)
    static const int MODE_CODE_VALID = 2; // correct code provided, will remain here 'til manually changing to IDLE (when disabling alarm)
    static const int MODE_CONFIG = 3; // modify the current time or alarm

    static const int NO_EVENT = -1; // must match the no-event code from adafruit keypad lib

    int VALID_CODE[4] = {1, 5, 8, 9}; // SECURITY LEVEL INCREASING
    
    int mode;
    unsigned long accumulated_millis;
    unsigned long current_long_click_millis;
  
    int current_key_pressed = Keypad::NO_KEY;
    int current_config_key_pressed = Keypad::NO_KEY;
    int current_code[4] = {-1, -1, -1, -1};
    int current_code_digit = 0;

    bool config_mode_requested = false; // change to config mode will be handled by the clock, here we're just setting the flag when proceed
    bool alarm_toggle_requested = false; // request to toggle alarm on/off, handled by the clock
    bool hour_format_toggle_requested = false; // request to change to am/pm, handled by the clock

    // maybe in the future we can use int? Since it uses the adafruit keypad lib we're using chars, so maybe we can change that...
    char KEYS[Keypad::ROWS][Keypad::COLUMNS] = {
      {'1','2','3'},
      {'4','5','6'},
      {'7','8','9'},
      {'*','0','#'}
    };
    
    void change_mode(int new_mode);
    int get_pressed_key();
    void set_led_level(int level);
    
};

#endif
