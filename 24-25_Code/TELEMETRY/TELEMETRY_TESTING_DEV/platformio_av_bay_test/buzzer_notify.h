#ifndef BUZZER_NOTIFY_H
#define BUZZER_NOTIFY_H
#include <Arduino.h>

class BuzzerNotify
{
public:
  BuzzerNotify(int pin);//constructor

  void Setup();
  void Check();
  void Trigger();
  void Reset();

private:
  int pin_number;
  bool beeping = false;
  const uint32_t MAX_CYCLES = 1800;
  const uint32_t MAX_CYCLES_ON = 1850;
  uint32_t curr_cycles = 1800;
};

#endif