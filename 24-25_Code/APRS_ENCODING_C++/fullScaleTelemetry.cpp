// Translated from EECS's previous script from Python to C++ By K Miller
// ax25_encode_call() translated by Jay H
// Description: Primary telemetry script for encoding and transmitting APRS AX.25 data

//
//  *********** UNFINISHED-- Need a digital bell 202 digital modem specification using pulse wave modulation in order to transmit ********
//

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#define TESTSTRING "The quick brown fox jumps over the lazy dog!!...sadfsa*****"
// #include "c++_radio_scripts/DacTone-main/DacTone.h"

// #include <Arduino.h>

using namespace std;
/*
import socket       // For establishing a TCP socket connection
import time         // For time delays
import RPi.GPIO as GPIO  // For controlling Raspberry Pi GPIO pins
import board
import busio
from adafruit_bmp3xx import BMP3XX_I2C
*/

// KISS protocol special characters
uint8_t KISS_FEND = 0xC0;     // Frame delimiter (start and end of a KISS frame)
uint8_t KISS_FESC = 0xDB;     // Escape character
uint8_t KISS_TFEND = 0xDC;    // Translated FEND (if FEND appears in data, it gets escaped)
uint8_t KISS_TFESC = 0xDD;    // Translated FESC (if FESC appears in data, it gets escaped)
uint8_t KISS_CMD_DATA = 0x00; // Command byte for sending data

/*
Encodes a payload in KISS format.

This function wraps the provided data in KISS protocol format,
adding the necessary frame delimiters and escaping special characters.
Parameters:
- data (bytes): The data payload to be encoded.

Returns:
- bytearray: The KISS-encoded frame.
*/
std::vector<uint8_t> kiss_encode(std::vector<uint8_t> data)
{
    std::vector<uint8_t> kiss_frame; // Byte Vector, stores sequence of uint8_t (bytes) to be transmitted

    kiss_frame.insert(kiss_frame.end(), {KISS_FEND, KISS_CMD_DATA}); // Add FEND and Command byte to start

    for (int i = 0; i < data.size(); i++)
    {
        uint8_t byte = kiss_frame[i];

        // Escape FEND character within data
        if (byte == KISS_FEND)
        {
            kiss_frame.insert(kiss_frame.end(), {KISS_FESC, KISS_TFEND});
        }
        // Escape FESC character within data
        else if (byte = KISS_FESC)
        {
            kiss_frame.insert(kiss_frame.end(), {KISS_FESC, KISS_TFESC});
        }
        // Regular byte, do not escape
        else
        {
            kiss_frame.push_back(byte);
        }

        // std::cout << byte;
    }
    // Add FEND to the end of the kiss frame
    kiss_frame.push_back(KISS_FEND);
    return kiss_frame;
}

/*
Encodes a callsign with SSID for AX.25 protocol.

The callsign is padded to 6 characters, and each character is shifted left by 1 bit
as required by the AX.25 specification. The SSID is also encoded with specific bits.

Parameters:
- call (str): The callsign (max 6 characters).
- ssid (int): The SSID number (0-15).
- last (bool): Indicates if this is the last address field (set the "last" bit).

Returns:
- bytearray: Encoded callsign and SSID.
*/
vector<uint8_t> ax25_encode_call(string call, int ssid, bool last = 0)
{
    while (call.length() < 6)
    { // Ensure the callsign is exactly 6 characters by padding with spaces
        call = call + " ";
    }

    for (int i = 0; i < call.length(); i++)
    { // shift each bit by 1 bit
        call[i] = call[i] << 1;
    }
    vector<uint8_t> encoded_call;
    for (int i = 0; i < call.length(); i++)
    {
        encoded_call.push_back(call[i]);
    }
    uint8_t ssidByte = ssid & (0x0F) << 1 | 0x60;

    if (last)
    {
        ssidByte |= 0x01;
    }
    encoded_call.push_back(ssidByte);

    return encoded_call;
}

/*
Creates an AX.25 frame with source, destination, and data.

Parameters:
- source (str): Source callsign.
- source_ssid (int): SSID for the source callsign.
- destination (str): Destination callsign.
- destination_ssid (int): SSID for the destination callsign.
- data (str): The payload (APRS message).

Returns:
- bytearray: Complete AX.25 frame with headers and data.
*/
std::vector<uint8_t> create_ax25_frame(std::string source, int source_ssid, std::string destination, int destination_ssid, std::string data)
{
    std::vector<uint8_t> ax25_frame;

    // Encode destination, callsign, and SSID
    std::vector<uint8_t> ax25_byte_array = ax25_encode_call(destination, destination_ssid);
    for (int i = 0; i < ax25_byte_array.size(); i++)
    {
        uint8_t byte = ax25_byte_array[i];
        ax25_frame.insert(ax25_frame.end(), byte);
    }

    // Encode Source callsign and SSID (last = True since it's the last address field)
    ax25_byte_array = ax25_encode_call(source, source_ssid, true);
    for (int i = 0; i < ax25_byte_array.size(); i++)
    {
        uint8_t byte = ax25_byte_array[i];
        ax25_frame.insert(ax25_frame.end(), byte);
    }
    // Control field: UI frame (Unnumbered Information)
    ax25_frame.push_back((uint8_t)0x03);
    // PID field: No Layer 3 protocol
    ax25_frame.push_back((uint8_t)0xF0);

    // Append payload data
    for (int i = 0; i < data.length(); i++)
    {
        // Cast data chars into uint8_ts
        ax25_frame.push_back((int)data[i]); // I do *not* know if the chars should be cast as byte literals or if I need to cast as (int) for decimal ascii
    }
    return ax25_frame;
}

/*
Encodes APRS data in AX.25, wraps in KISS format, and sends it to REMOVEDIREWOLF via TCP.

This function also activates a GPIO pin on the Raspberry Pi before sending.

Parameters:
- aprs_data (str): The APRS message to be sent.
- source (str): Source callsign.
- source_ssid (int): SSID for the source callsign.
- destination (str): Destination callsign.
- destination_ssid (int): SSID for the destination callsign.
- tcp_ip (str): IP address for the REMOVEDIREWOLF server (default: 127.0.0.1).
- tcp_port (int): TCP port for the REMOVEDIREWOLF server (default: 8001).
*/
void send_aprs_packet(std::string aprs_data, std::string source, int source_ssid, std::string destination, int destination_ssid,
                      std::string tcp_ip = "127.0.0.1", int tcp_port = 8001)
{
    std::vector<uint8_t> ax25_frame = create_ax25_frame(source, source_ssid, destination, destination_ssid, aprs_data);
    std::vector<uint8_t> kiss_frame = kiss_encode(ax25_frame);

/*
//Do we need to *do* any of this for esp? Doesn't *seem* like it?
    //Configure GPIO pins
    // Set GPIO mode to broadcom pin numbering
    // Define pin number to control output
    // Set that pin as a GPIO output

    // Activate GPIO pin to signal packet transmission
*/
// Declare mic pin as D9
// const int mic_pin = D9;

    // Bell 202 modem function for sending frame
    for (int i = 0; i < kiss_frame.size(); i++) {
        uint8_t byte = kiss_frame[i];



    }
}

/*
    python version of script:
   def send_aprs_packet(aprs_data, source, source_ssid, destination, destination_ssid, tcp_ip="127.0.0.1", tcp_port=8001):
       // Create the AX.25 frame with the given data
       ax25_frame = create_ax25_frame(source, source_ssid, destination, destination_ssid, aprs_data)
       // Wrap the AX.25 frame in KISS encoding
       kiss_frame = kiss_encode(ax25_frame)


       // Configure Raspberry Pi GPIO
       GPIO.setmode(GPIO.BCM)  // Set GPIO mode to Broadcom pin numbering
       pin = 17                // Pin number to control (GPIO 17)
       GPIO.setup(pin, GPIO.OUT)  // Set GPIO pin as an output

       // Activate GPIO pin to signal packet transmission
       GPIO.output(pin, GPIO.HIGH)

       // Send the KISS frame to REMOVEDIREWOLF over TCP
       with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
           s.connect((tcp_ip, tcp_port))
           s.sendall(kiss_frame)  // Send the KISS frame
       time.sleep(15)  // Wait for 15 seconds (to ensure packet transmission completes)
       GPIO.cleanup()  // Clean up GPIO pins after use
*/

std::string source = "KQ4FYU";    // Source callsign
int source_ssid = 1;              // Source SSID
std::string destination = "APRS"; // Destination callsign
int destination_ssid = 0;         // Destination SSID
                                  /*
                                     // APRS packet details
                                       // Example APRS payload
                                  
                                  
                                     // Continuously send APRS packets every 10 seconds
                                     def get_apogee():
                                         last_altitude = bmp.altitude
                                         while True:
                                             pressure = bmp.pressure
                                             temperature = bmp.temperature
                                             altitude = bmp.altitude
                                  
                                             if altitude > last_altitude:
                                                 last_altitude = altitude
                                             elif altitude < last_altitude:
                                                 apogee_reached = True
                                                 return bmp.altitude
                                  
                                     apogee = get_apogee()
                                     aprs_data = f"!Highest apogee = {apogee}"
                                  
                                     for i in range(10):
                                         send_aprs_packet(aprs_data, source, source_ssid, destination, destination_ssid)
                                  
                                     // Wait for the REMOVEDIREWOLF process to finish (blocks indefinitely)
                                     REMOVEDIREWOLF_process.wait()
                                     */

//Prints the hexadecimal of a byte array                                    
void printHex(std::vector<uint8_t> data)
{
    for (uint8_t byte : data)
    {
        //Change cout format to hex and print
        std::cout << "0x" << std::hex << (int)byte << " ";
    }
    std::cout << std::dec << "\n"; // Reset format back to decimal
}

// Main function for testing the C++ translation
int main()
{
    std::cout << "Hi, main is running\n";
    std::string aprs_data = TESTSTRING;
    std::string source = "KQ4FYU";    // Source callsign
    int source_ssid = 1;              // Source SSID
    std::string destination = "APRS"; // Destination callsign
    int destination_ssid = 0;         // Destination SSID

    std::vector<uint8_t> ax25_frame = create_ax25_frame(source, source_ssid, destination, destination_ssid, aprs_data);

    printHex(ax25_frame);
    std::cout << "\n";

    /*
    // Dummy test data
    std::vector<uint8_t> datalist = {0x21, 0xBB, 0x21};
    datalist = kiss_encode(datalist);

    // Print the bytelist
    for (int i = 0; i < datalist.size(); i++)
    {
        printf("%02X", datalist[i]);

        // std::cout << unsigned(datalist[i]);
    }
    std::cout << "\n";

    // Test string for examining char to uint8_t or int casts
    std::string StringToByteTest = "This should look the same";
    std::vector<uint8_t> ASCIITEST;
    // Append data to the uint8_t list
    for (int i = 0; i < StringToByteTest.length(); i++)
    {
        // Cast data chars into uint8_ts
        ASCIITEST.push_back((int)StringToByteTest[i]);
    }
    // Print the ASCIITEST int list-- should print as decimal ascii. It's unformatted when printed like this, though, so have fun
    for (int i = 0; i < ASCIITEST.size(); i++)
    {
        std::cout << unsigned(ASCIITEST[i]);
    }
    std::cout << "\n";
    */

    return 0;
}