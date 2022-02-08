#include "Arduino.h"

class CapacitiveSensorMinimal {
  public:
    unsigned int dischargeTime = 50;

    CapacitiveSensorMinimal(int pin);
    void begin();
    uint16_t read();
    uint16_t readHiRes();
    uint16_t readHiRes(uint16_t readings);
    bool compare(uint16_t thresh);
  private:
    int _pin;
    byte _mask;
    volatile byte* _DDR;
    volatile byte* _PORT;
    volatile byte* _PIN;
};