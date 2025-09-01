#ifndef SOAR_SPEAKER_H
#define SOAR_SPEAKER_H

#include <Audio.h>

class SOAR_Speaker {
public:
    SOAR_Speaker(int I2S_DIN, int I2S_BCLK, int I2S_LRC, int volume);
    void playSong(char *title);
    void playR2D2(char *number);
    void stop();
    void setVolume(int volume);
private:
    Audio audio;
    int _i2s_din;
    int _i2s_blck;
    int _i2s_lrc;
    int _volume;
};

#endif // SOAR_SPEAKER_H
