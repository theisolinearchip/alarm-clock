#ifndef Two_keys_h
#define Two_keys_h

class Two_keys {
  public:
    
    //constructor
    Two_keys::Two_keys(int left_key_pin, int left_key_led_pin, int right_key_pin, int right_key_led_pin);

    void init();
    void idle();
    bool is_final_valid_status();
    void update(unsigned long current_accumulated_millis);
    
  private:

    static const unsigned long MILLIS_RESET_INTERVAL = 250; // 0.25 sec off 'til reset
    static const unsigned long MILLIS_KEYS_INTERVAL = 250; // 0.5 sec between activations

    static const int MODE_RESET = -1; // the system needs a reset (both two keys must be off for MILLIS_RESET_INTERVAL time)
    static const int MODE_IDLE = 0; // do nothing
    static const int MODE_OFF = 1; // no keys has been activated
    static const int MODE_ONE = 2; // one key activated, will countdown and then go to fail unless the second one is activated before MILLIS_KEYS_INTERNAL
    static const int MODE_TWO = 3; // both keys activated, won't change unless manually trigger "init()" function again

    int mode;
    unsigned long accumulated_millis;
  
    int left_key_pin;
    int left_key_led_pin;
    int right_key_pin;
    int right_key_led_pin;

    void change_mode(int new_mode);

};

#endif
