#include <CapacitiveSensorMinimal.h>

//Connect a wire to pin 8 and touch the other end
CapacitiveSensorMinimal sensor(8);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  sensor.begin();
}

void loop() {
  //readHiRes has a resolution of 1 clock cycle, but is slower
  int val = sensor.readHiRes();
//  int val = sensor.readHiRes(16); //Spesify how many samples to take
  Serial.print(val);
  Serial.print('\t');

  //read has a resolution of 6 clocks, but is faster
  val = sensor.read();
  Serial.print(val);
  Serial.println();

  //Compare has a resolution of 1 clock, and is fast
  if (sensor.compare(25)) {
    digitalWrite(LED_BUILTIN,  HIGH);
  } else {
    digitalWrite(LED_BUILTIN,  LOW);
  }
  
  delay(10);
}