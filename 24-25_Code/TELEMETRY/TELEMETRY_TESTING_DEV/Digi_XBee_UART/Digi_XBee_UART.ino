#include <XBeeArduino.h>
#include <config.h>
#include <port.h>
#include <xbee.h>
#include <xbee_api_frames.h>
#include <xbee_at_cmds.h>
#include <xbee_S3B.h>
#include <stdarg.h> // Use C-style header instead of C++ <cstdarg>
#include <Arduino.h>
#include "globals.h"

// Declare a pointer for the serial port
HardwareSerial mySerial(0);
Stream* serialPort = &mySerial;

// Declare XBeeArduino instance
XBeeArduino* xbee;

// XBee payload to send

// Declare the payload if not declared elsewhere:
// uint8_t examplePayload[] = { 0x48, 0x65, 0x6C, 0x6C, 0x6F }; // Example data; adjust as needed

// uint16_t payloadLen;

/**
 * @brief Callback function triggered when data is received from the XBee module.
 *
 * This function is called when the XBee module receives data. It processes the
 * incoming data, extracts relevant information, and handles it accordingly.
 *
 * @param[in] data Pointer to the received packet data.
 */
void OnReceiveCallback(void* data) {
  Serial.println("Onreceive callback executed");
  // xbee_api_frame_t* frame = (xbee_api_frame_t*)data;

  // if (apiReceiveApiFrame(xbee->getXBee(), (xbee_api_frame_t*)data) == API_RECEIVE_SUCCESS) // Correct type conversion
  // {
  //   apiHandleFrame(xbee->getXBee(), *(xbee_api_frame_t*)data); // Correct type conversion
  // }
}

/**
 * @brief Callback function triggered after data is sent from the XBee module.
 *
 * This function is called when the XBee module completes sending data. It handles
 * any post-send operations, such as logging the transmission status or updating
 * the state of the application.
 *
 * @param[in] data Pointer to the sent packet data.
 */
void OnSendCallback(void* data) {
  Serial.println("Send callback initiated");
  // Handle post-send operations here
}

void setup() {
  Serial.begin(115200);
  // serialPort = &Serial1;
  // serialPort.begin(9600);
  // Instead of creating a new HardwareSerial, use the built-in Serial1
  // serialPort = &Serial1;
  // Serial1.begin(9600);
  Serial.println("Debug Point 0");
  // Caution: watch out with the XBEE_STANDARD thing, since there's custom adjustment code for XBEE_LORA
  xbee = new XBeeArduino(serialPort, 115200, XBEE_STANDARD, OnReceiveCallback, OnSendCallback);

  Serial.println("Debug Point 0A");
  // if (!xbee->begin(mySerial))
  // {
  //   Serial.println("Failed to initialize XBee");
  //   while (1)
  //     ; // Halt the program on failure
  // }

  mySerial.begin(115200, SERIAL_8N1, -1, -1);
}

void loop() {
  Serial.println("Debug Point A");
  xbee->process();
  delay(500);
  // Check if 10 seconds have passed
  // static uint32_t startTime = millis();
  // if (millis() - startTime >= 10000) {
  //   // Send data if connected, else reconnect
  //   Serial.println("Debug Point B");

  //   if (xbee->isConnected()) {
  //     // Sending ----------------------------------------------------------------------------------
  //     Serial.print("Sending 0x");
  //     delay(1000);
  //     // for (int i = 0; i < payloadLen; i++)
  //     // {
  //     //   Serial.print(examplePayload[i], HEX);
  //     // }
  //     // Serial.println();

  //     // Send the data --------------------------
  //     // xbee_api_frame_t api_frame;
  //     // api_frame.type = XBEE_API_TYPE_TX_REQUEST;
  //     // // 01 00 00 00 00 00 00 FF FF FF FE 00 00 47 6F 6F 64 20 6D 6F 72 6E 69 6E 67
  //     // uint8_t examplePayload[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x47, 0x6F, 0x6F, 0x64, 0x20, 0x65, 0x76, 0x65, 0x6E, 0x69, 0x6E, 0x67, 0x20, 0x68, 0x75, 0x6D, 0x61, 0x6E, 0x73 };;
  //     // api_frame.length = sizeof(examplePayload) / sizeof(examplePayload[0]);
  //     // // Replace strncpy with memcpy to copy binary data correctly.
  //     // memcpy(api_frame.data, examplePayload, api_frame.length);
  //     // api_frame.checksum = xbee->getChecksum(api_frame.data, api_frame.length);

  //     // if (!xbee->completeTx("Connor is crazy."))
  //     // {
  //     //   Serial.println("Failed to send data.");
  //     // }
  //     // else
  //     // {
  //     //   Serial.println("Data sent successfully.");
  //     // }




  //     // Receiving --------------------------------------------------------------------------------
  //     // uint32_t rxtime = millis();
  //     // while (millis() - rxtime < 5000)
  //     // {
  //     //   xbee->handleRxPacket();
  //     //   delay(100);
  //     // }

  //     // end of the instructions
  //   }
  //   else {
  //     Serial.println("Not connected. Connecting...");
  //     if (!xbee->connect()) {
  //       Serial.println("Failed to connect.");
  //     }
  //     else {
  //       Serial.println("Connected!");
  //     }
  //   }
  //   // Reset the start time
  //   startTime = millis();
  // }
  // else {
  //   Serial.println("Wait period is not over");
  //   delay(1000);
  // }
}