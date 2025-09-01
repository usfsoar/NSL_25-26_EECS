#include "V2_SOAR_SPEAKER.h"


SOAR_Speaker speaker;

setup () {
    Serial.begin(115200);
    speaker.playMario();

}

loop () {

}