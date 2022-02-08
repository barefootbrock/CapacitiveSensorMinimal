#include "Arduino.h"
#include "CapacitiveSensorMinimal.h"

CapacitiveSensorMinimal::CapacitiveSensorMinimal(int pin) {
  _pin = pin;
}

void CapacitiveSensorMinimal::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _mask = digitalPinToBitMask(_pin); //Get mask for pin in registor
  _DDR = portModeRegister(digitalPinToPort(_pin)); //Get pointer to DDR registor
  _PORT = portOutputRegister(digitalPinToPort(_pin)); //Get pointer to PORT registor
  _PIN = portInputRegister(digitalPinToPort(_pin)); //Get pointer to PIN registor
}

uint16_t CapacitiveSensorMinimal::read() {
  //Read how many clock cycles it takes to charge the pin
  //Resolution is 6 clocks, Overflows after 1527 clocks (255*6-3)
  byte count = 0;
  byte oldSREG = SREG; //Save interrupt flag

  *_DDR |= _mask; //pinMode(pin, OUTPUT);
  *_PORT &= ~_mask; //digitalWrite(pin, LOW);
  delayMicroseconds(dischargeTime); //discharge capactor

  noInterrupts();
  *_DDR &= ~_mask; //pinMode(_pin, INPUT);
  //Finds min n so that charge time <= 6 * n - 3
  asm volatile( //read pin as fast as posable
    "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT) enable pullup

    "checkPin:         \n" //This loop adds 1 to count while pin is LOW (each loop is 6 clocks)
    "inc %[count]      \n" //count++; (1 clock)
    "ld r24, X         \n" //Load _PIN into r24 (2 clocks)
    "and r24, %[mask]  \n" //r24 &= mask (Zero flag is set if result is 0 meanimg pin is LOW) (1 clock)
    "breq checkPin     \n" //If Zero is set loop back to top (2 clocks)
    : [count] "+r" (count)
    : [mask] "r" (_mask),
    [PORTset] "r" (*_PORT | _mask),
    "x" (_PIN),
    "z" (_PORT)
    : "r24"
  );
  SREG = oldSREG; //Enable interrupts if they were before

  *_DDR |= _mask; //pinMode(pin, OUTPUT);
  *_PORT &= ~_mask; //digitalWrite(pin, LOW);
  return count * 6 - 3; //return num clocks (each asm loop is 6 clocks)
}

uint16_t CapacitiveSensorMinimal::readHiRes() {
  return readHiRes(8);
}

uint16_t CapacitiveSensorMinimal::readHiRes(uint16_t readings) {
  //Read capacitance several times and average (accurate to 1 clock)
  uint32_t total = 0;

  for (uint16_t i = 0; i < readings; i++) {
    uint16_t val = read(); //Accurate to 6 clocks
    while (!compare(val - 1)) val--; //Accurate to 1 clock

    total += val;
  }

  return total / readings;
}

bool CapacitiveSensorMinimal::compare(uint16_t thresh) {
  //Test if pin takes longer than thresh clocks to charge
  //Accurate to 1 clock cycle! Min is 2 clocks, Max is 772 (255*3+7) clocks

  byte portVal, threshByte;
  byte oldSREG = SREG; //Save interrupt flag

  if (thresh < 2) return true; //No delay less than 2 clocks is possable
  if (thresh > 722) return false; //No delay less than 2 clocks is possable

  *_DDR |= _mask; //pinMode(pin, OUTPUT);
  *_PORT &= ~_mask; //digitalWrite(pin, LOW);
  delayMicroseconds(dischargeTime); //discharge capactor

  //Many asm code variations for different delay length
  if (thresh == 2) { //Read after 2 clocks
    noInterrupts();
    *_DDR &= ~_mask; //pinMode(_pin, INPUT);
    asm volatile( //read pin as fast as posable
      "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT)
      "ld %[portVal], X  \n" //portVal = *_PIN (2 clocks)
      : [portVal] "=r" (portVal)
      : [thresh] "d" (threshByte),
      [PORTset] "r" (*_PORT | _mask),
      "x" (_PIN),
      "z" (_PORT)
      :
    );
    SREG = oldSREG; //Enable interrupts if they were before

  } else if (thresh == 3) { //Read after 3 clock
    noInterrupts();
    *_DDR &= ~_mask; //pinMode(_pin, INPUT);
    asm volatile( //read pin as fast as posable
      "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT)
      "nop               \n" //no operation (1 clock)
      "ld %[portVal], X  \n" //portVal = *_PIN (2 clocks)
      : [portVal] "=r" (portVal)
      : [thresh] "d" (threshByte),
      [PORTset] "r" (*_PORT | _mask),
      "x" (_PIN),
      "z" (_PORT)
      :
    );
    SREG = oldSREG; //Enable interrupts if they were before

  } else if (thresh == 4) { //Read after 4 clocks
    noInterrupts();
    *_DDR &= ~_mask; //pinMode(_pin, INPUT);
    asm volatile( //read pin as fast as posable
      "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT)
      "nop               \n" //no operation (1 clock)
      "nop               \n" //no operation (1 clock)
      "ld %[portVal], X  \n" //portVal = *_PIN (2 clocks)
      : [portVal] "=r" (portVal)
      : [thresh] "d" (threshByte),
      [PORTset] "r" (*_PORT | _mask),
      "x" (_PIN),
      "z" (_PORT)
      :
    );
    SREG = oldSREG; //Enable interrupts if they were before

  } else if (thresh % 3 == 2) { //Read after 5 + 3n clocks
    threshByte = (thresh - 5) / 3;

    noInterrupts();
    *_DDR &= ~_mask; //pinMode(_pin, INPUT);
    asm volatile( //read pin as fast as posable
      "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT)

      "waitLoopA:        \n" //Wait threshold * 3 clocks (loop is 3 clocks)
      "subi %[thresh], 1 \n" //threshold -= 1; (1 clock)
      "brcc waitLoopA    \n" //Restart loop if threshold > 0 (2 clocks)
      "ld %[portVal], X  \n" //portVal = *_PIN (2 clocks)
      : [portVal] "=r" (portVal)
      : [thresh] "d" (threshByte),
      [PORTset] "r" (*_PORT | _mask),
      "x" (_PIN),
      "z" (_PORT)
      :
    );
    SREG = oldSREG; //Enable interrupts if they were before

  } else if (thresh % 3 == 0) { //Read after 6 + 3n clocks
    threshByte = (thresh - 6) / 3;

    noInterrupts();
    *_DDR &= ~_mask; //pinMode(_pin, INPUT);
    asm volatile( //read pin as fast as posable
      "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT)
      "nop               \n" //no operation (1 clock)

      "waitLoopB:        \n" //Wait threshold * 3 clocks (loop is 3 clocks)
      "subi %[thresh], 1 \n" //threshold -= 1; (1 clock)
      "brcc waitLoopB    \n" //Restart loop if threshold > 0 (2 clocks)
      "ld %[portVal], X  \n" //portVal = *_PIN (2 clocks)
      : [portVal] "=r" (portVal)
      : [thresh] "d" (threshByte),
      [PORTset] "r" (*_PORT | _mask),
      "x" (_PIN),
      "z" (_PORT)
      :
    );
    SREG = oldSREG; //Enable interrupts if they were before

  } else if (thresh % 3 == 1) { //Read after 7 + 3n clocks
    threshByte = (thresh - 7) / 3;

    noInterrupts();
    *_DDR &= ~_mask; //pinMode(_pin, INPUT);
    asm volatile( //read pin as fast as posable
      "st Z, %[PORTset]  \n" //digitalWrite(_pin, HIGH); Store new PORT value in address Z (_PORT)
      "nop               \n" //no operation (1 clock)
      "nop               \n" //no operation (1 clock)

      "waitLoopC:        \n" //Wait threshold * 3 clocks (loop is 3 clocks)
      "subi %[thresh], 1 \n" //threshold -= 1; (1 clock)
      "brcc waitLoopC    \n" //Restart loop if threshold > 0 (2 clocks)
      "ld %[portVal], X  \n" //portVal = *_PIN (2 clocks)
      : [portVal] "=r" (portVal)
      : [thresh] "d" (threshByte),
      [PORTset] "r" (*_PORT | _mask),
      "x" (_PIN),
      "z" (_PORT)
      :
    );
    SREG = oldSREG; //Enable interrupts if they were before

  }

  *_DDR |= _mask; //pinMode(pin, OUTPUT);
  *_PORT &= ~_mask; //digitalWrite(pin, LOW);
  return (portVal & _mask) ? false : true; //Return true if pin was low after delay
}