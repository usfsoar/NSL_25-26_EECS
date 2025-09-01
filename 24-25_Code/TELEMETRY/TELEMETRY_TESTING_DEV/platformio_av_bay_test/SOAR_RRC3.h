#ifndef SOAR_RRC3_H
#define SOAR_RRC3_H

#include <HardwareSerial.h>
class SOAR_RRC3{
public:
    SOAR_RRC3(int serial_bus, int rx, int tx);
    void setup();
    void GET_ALTITUDE(float *altitude, bool *ready, bool *failed);
private:
    HardwareSerial* altSerial;
    int RX_PIN;
    int TX_PIN;
    uint32_t MAX_TIME;
    int fail_count;

};
#endif

