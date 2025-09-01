#include "DISPLAY_H"

void Display::Initialize()
{
    if (!this->lcd.begin)
    {
        Serial.print("Could not find a valid display T_T");
        return;
    }
    this->lcd.backlight();
}

Display::Display()
{
    Serial.print("Display object created");
}

void print(string message)
{
    this->lcd.print(message);
}
