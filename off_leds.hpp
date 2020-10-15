#ifndef Off_leds_h
#define Off_leds_h

class Off_leds {
  public:
    
    //constructor
    Off_leds::Off_leds(int off_leds_size, int *off_leds_pins);

    void init();
    void set_off_led_level(int level);
    
  private:

    int off_leds_size;
    int *off_leds_pins;
    
};

#endif
