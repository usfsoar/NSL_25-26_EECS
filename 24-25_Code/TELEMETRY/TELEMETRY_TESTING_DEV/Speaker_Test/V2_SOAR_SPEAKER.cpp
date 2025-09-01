#include "V2_SOAR_SPEAKER.h"

/*
// Notes frequencies for reference
float frequencies[] = {
  0,    // Rest 0
  261,  // C4 1
  277,  // C#4/Db4 2
  293,  // D4 3
  311,  // D#4/Eb4 4
  329,  // E4 5
  349,  // F4 6
  369,  // F#4/Gb4 7
  392,  // G4 8
  415,  // G#4/Ab4 9
  440,  // A4 10
  466,  // A#4/Bb4 11
  493,  // B4 12
  523,  // C5 13
  554,  // C#5/Db5 14
  587,  // D5 15
  622,  // D#5/Eb5 16
  659,  // E5 17
  698,  // F5 18
  740,  // F#5/Gb5 19
  784,  // G5 20
  830,  // G#5/Ab5 21
  880,  // A5 22
  932,  // A#5/Bb5 23
  988,  // B5 24
  1047  // C6 25
};
*/

SOAR_Speaker::SOAR_Speaker() {
  AudioConnection patchCord1(sine1, 0, audioOutput, 0);
  AudioConnection patchCord2(sine1, 0, audioOutput, 1);

  AudioMemory(12);
  for (int i = 0; i < 26; i++) {
      frequencies[i] = 261.63 * pow(2, (i / 12.0));
  }
}

void SOAR_Speaker::playMario() {
    int themeData[][2] = {
        {17, 150}, {17, 150}, {0, 150}, {17, 150},
        {0, 150}, {13, 150}, {17, 150}, {0, 150},
        {20, 150}, {0, 500}, {8, 150}, {0, 500},
        {13, 150}, {0, 300}, {8, 150}, {0, 150},
        {5, 150}, {0, 150}, {10, 150}, {12, 150},
        {11, 150}, {10, 150}, {0, 150}, {8, 150},
        {17, 150}, {0, 150}, {20, 150}, {0, 150},
        {22, 150}, {0, 150}, {18, 150}, {20, 150},
        {0, 150}, {17, 150}, {0, 150}, {13, 150},
        {15, 150}, {12, 150}, {0, 150}, {12, 150}//....
    };
    int themeLength = sizeof(themeData) / sizeof(themeData[0]);
    Note marioTheme[themeLength];

    for (int i = 0; i < static_cast<int>(sizeof(themeData) / sizeof(themeData[0])); i++) {
        marioTheme[i].frequency = frequencies[themeData[i][0]];
        marioTheme[i].duration = themeData[i][1];
    }

    for (int i = 0; i < themeLength; i++) {
        tone(speaker_pin, marioTheme[i].frequency);
        delay(marioTheme[i].duration);
        noTone(speaker_pin);
    }
}

void SOAR_Speaker::stop() {
    noTone(speaker_pin);
}
