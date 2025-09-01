#include "SOAR_Speaker.h"

SOAR_Speaker::SOAR_Speaker(int I2S_DIN, int I2S_BCLK, int I2S_LRC, int volume)
    : _i2s_din(I2S_DIN), _i2s_blck(I2S_BCLK), _i2s_lrc(I2S_LRC), _volume(volume) {}

void SOAR_Speaker::playSong(char *title) {
    char path[50] = "/songs/";
    strcat(path, title);
    audio.connecttoFS(SD, path);
}

void SOAR_Speaker::playR2D2(char *number) {
    char path[50] = "/R2D2/";
    strcat(path, number);
    audio.connecttoFS(SD, path);
}

void SOAR_Speaker::stop() {
    if (audio.isRunning()) {
        audio.stopSong();
    }
}


void SOAR_Speaker::setVolume(int volume) {
    _volume = volume;
}
