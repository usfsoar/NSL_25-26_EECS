#ifndef DISPLAY_H
#define DISPLAY_H
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class Display
{
public:
    Display();
    void Initialize();
    void print(string message);
    LiquidCrystal_I2C lcd(0x27, 16, 2);

private:
}

#endif