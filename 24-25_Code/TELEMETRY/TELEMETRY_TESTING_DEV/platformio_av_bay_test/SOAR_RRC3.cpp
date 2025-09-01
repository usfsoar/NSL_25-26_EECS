#include "SOAR_RRC3.h"

SOAR_RRC3::SOAR_RRC3(int serial_bus, int rx, int tx) :
   RX_PIN(rx), TX_PIN(tx) {
    altSerial = new HardwareSerial(0);
   }

void SOAR_RRC3::setup(){
    altSerial->begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void SOAR_RRC3::GET_ALTITUDE(float *altitude, bool *ready, bool *failed){
    if (!altSerial->available()){
        *ready = false;
        *altitude = -60000;  // Use an impossible value to indicate failure.
        return;
    }

    *ready = true;  // Ready to read data.
    String result = altSerial->readStringUntil('\r');  // Read until carriage return.

    if(result.length() == 0){
        *failed = true;
        return;
    }

    int firstComma = result.indexOf(',');  // Find the first comma.
    if(firstComma == -1) {
        *failed = true;
        return;
    }
    int secondComma = result.indexOf(',', firstComma + 1);  // Find the second comma.
    if(secondComma == -1) {
        *failed = true;
        return;
    }

    String string_alt = result.substring(firstComma + 1, secondComma);  // Get the substring for altitude.
    if (string_alt.length() == 0) {
        *failed = true;
        return;
    }

    *altitude = strtol(string_alt.c_str(), NULL, 10);  // Convert the altitude string to a float using c_str().
    *failed = false;  // Indicate success if no errors encountered.
}



