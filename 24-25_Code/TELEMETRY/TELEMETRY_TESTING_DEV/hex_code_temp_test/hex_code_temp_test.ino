#include <Arduino.h>
#include <string.h> // For memcpy
#include "hex_functions.h"

void setup() {
  Serial.begin(9600);
  Serial.println("Temporary hex code testing begins");
}

void loop() {
  myhexfunc("datastring");
}