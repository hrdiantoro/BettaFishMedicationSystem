#include "PCF8574.h"

PCF8574 buttonI2C(0x20);

void setup() {
  Serial.begin(115200);
  buttonI2C.pinMode(P1, INPUT_PULLUP);
  buttonI2C.pinMode(P2, INPUT_PULLUP);
  buttonI2C.begin();
}

void loop() {
  Serial.print("| buttonI2C.digitalRead(P1): ");
  Serial.print(buttonI2C.digitalRead(P1));
  Serial.print("| buttonI2C.digitalRead(P2): ");
  Serial.print(buttonI2C.digitalRead(P2));
  Serial.println();
}
