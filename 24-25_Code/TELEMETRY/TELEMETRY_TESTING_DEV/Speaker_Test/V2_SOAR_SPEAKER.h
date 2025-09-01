#ifndef SOAR_SPEAKER_H
#define SOAR_SPEAKER_H

#include <Audio.h>
#include <math.h>
#include <Arduino.h>

class SOAR_Speaker {
public:
    SOAR_Speaker();
    void playMario();
    void stop();
private:
    AudioSynthWaveformSine sine1;
    AudioOutputI2S audioOutput;
    float frequencies[26];
    struct Note {
        float frequency;
        int duration;
    };
    int speaker_pin = 7;
};

#endif // SOAR_SPEAKER_H
