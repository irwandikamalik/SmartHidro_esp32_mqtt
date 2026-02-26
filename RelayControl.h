#ifndef RelayControl_h
#define RelayControl_h

#include <Arduino.h>

class RelayControl {
  private:
    int pin;
    bool state;

  public:
    RelayControl(int relayPin);
    void begin();
    void on();
    void off();
    void setState(bool newState);
    bool getState();
};

#endif
