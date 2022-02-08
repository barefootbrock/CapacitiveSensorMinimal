# CapacitiveSensorMinimal Library

An alternative to the CapacitiveSensor that is much faster, requires only one pin, and does not need any external components. All that is needed is a single wire connected to any pin.

To take a measurement, the pin is first grounded to remove any charge. It is then connected to the Arduino's internal pull-up resistors and the time (in clock cycles) it takes for the pin to go high is measered. The timing loops are written in assembly, so thay are extremely fast. The compare and readHiRes functions have a resolution of one clock cycle (62.5nS on the Uno)! The read function is faster than readHiRes, but has a resolution of 6 clocks.

I have tested this on Arduino Uno and Mega, but any board with a similar architecture should work.
