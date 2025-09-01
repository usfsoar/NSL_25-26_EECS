#include "buzzer_notify.h"

BuzzerNotify::BuzzerNotify(int pin) : pin_number(pin){};

void BuzzerNotify::Setup()
{
    pinMode(pin_number, OUTPUT);
}
void BuzzerNotify::Check()
{
    curr_cycles++;
    #if DEBUG_BUZZ
        Serial.print("Cycles: ");
        Serial.print(curr_cycles);
        Serial.print("/");
        Serial.println(MAX_CYCLES);
    #endif
        if (curr_cycles > MAX_CYCLES && !beeping)
        {
            digitalWrite(pin_number, HIGH);
            beeping = true;
            Serial.println("Buzz!");
        }
        if (curr_cycles > MAX_CYCLES_ON)
        {
            digitalWrite(pin_number, LOW);
            beeping = false;
            Reset();
        }
  };
void BuzzerNotify::Trigger()
{
    if (!beeping)
    {
      digitalWrite(pin_number, HIGH);
      delay(50);
      digitalWrite(pin_number, LOW);
    }
}
void BuzzerNotify::Reset()
{
    curr_cycles = 0;
}