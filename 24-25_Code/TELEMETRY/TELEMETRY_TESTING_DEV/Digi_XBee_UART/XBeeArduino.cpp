#include "XBeeArduino.h"
#include "xbee.h"
#include "xbee_api_frames.h"
#include "port.h"
#include "xbee_S3B.h" // Include the header for XBeeS3BCreate

/**
 * @file XBeeArduino.cpp
 * @brief Implementation of the XBeeArduino class for Arduino-compatible XBee modules.
 *
 * This file contains the implementation of the XBeeArduino class, which provides an interface
 * for interacting with XBee modules on an Arduino platform.
 *
 * @version 1.0
 * @date 2024-08-17
 * author Felix Galindo
 *
 * @license MIT
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

XBeeArduino* XBeeArduino::instance_ = nullptr;

/**
 * @brief Constructor for XBeeArduino class.
 *
 * Initializes the XBee module based on the type and sets up callbacks and UART communication.
 *
 * @param serialPort A pointer to a Stream object (HardwareSerial or SoftwareSerial).
 * @param baudrate The baud rate for UART communication.
 * @param moduleType The type of XBee module (standard or LoRa).
 * @param onReceiveCallback A callback function to handle received data.
 * @param onSendCallback A callback function to handle post-send events.
 */
XBeeArduino::XBeeArduino(Stream* serialPort, uint32_t baudrate, XBeeModuleType moduleType,
    void (*onReceiveCallback)(void*),
    void (*onSendCallback)(void*))
    : serialPort_(serialPort), moduleType_(moduleType), xbee_(nullptr), baudRate_(baudrate),
    onReceiveCallback_(onReceiveCallback), onSendCallback_(onSendCallback) {

    if (moduleType_ == XBEE_STANDARD) {
        // Initialization for standard XBee modules
        instance_ = this;
        ctable_.OnReceiveCallback = onReceiveWrapper,
            ctable_.OnSendCallback = onSendWrapper,
            htable_.PortUartRead = portUartRead,
            htable_.PortUartWrite = portUartWrite,
            htable_.PortMillis = portMillis,
            htable_.PortFlushRx = PortFlushRx,
            htable_.PortUartInit = portUartInit,
            htable_.PortDelay = portDelay,
            xbee_ = (XBee*)XBeeS3BCreate(&ctable_, &htable_); // Correct function name
    }
    else if (moduleType_ == XBEE_LORA) {
        instance_ = this;
        ctable_.OnReceiveCallback = onReceiveWrapper,
            ctable_.OnSendCallback = onSendWrapper,
            htable_.PortUartRead = portUartRead,
            htable_.PortUartWrite = portUartWrite,
            htable_.PortMillis = portMillis,
            htable_.PortFlushRx = PortFlushRx,
            htable_.PortUartInit = portUartInit,
            htable_.PortDelay = portDelay,

            xbee_ = (XBee*)XBeeLRCreate(&ctable_, &htable_);
    }
}

/**
 * @brief Destructor for XBeeArduino class.
 *
 * Ensures proper disconnection and cleanup of the XBee module.
 */
XBeeArduino::~XBeeArduino() {
    if (xbee_ != nullptr) {
        XBeeDisconnect(xbee_);
        delete xbee_; // Free the memory allocated for the XBee structure
    }
}

/**
 * @brief Initializes the XBee module.
 * @return True if initialization is successful, otherwise false.
 */
 // DO NOT CALL THIS FUNCTIOn to setup a serial port - just define serial yourself in the .ino file
bool XBeeArduino::begin() {
    // if (xbee_ == nullptr)
    // {
    //     return false; // or handle error appropriately
    // }
    // uint8_t value = portUartInit(baudRate_, serialPort_);
    // Serial.print("status value of portuartinit: ");
    // Serial.println(value);
    // return value == 0;  // Corrected baudRate_ reference
    // return portUartInit(baudRate_, serialPort_) == 0;  // Corrected baudRate_ reference

    // return XBeeInit(xbee_, baudRate_, serialPort_);
    // HardwareSerial(1).begin(baudRate_, SERIAL_8N1, -1, -1);
    return true;
    // return XBeeInit(xbee_, baudRate_, serialPort_);
}

/**
 * @brief Connects the XBee module to the network.
 * @return True if the connection is successful, otherwise false.
 */
bool XBeeArduino::connect() {
    if (xbee_ != nullptr) {
        return XBeeConnect(xbee_);
    }
    return false;
}

/**
 * @brief Lets the XBee class process
 * @return void
 */
void XBeeArduino::process() {
    if (xbee_) {
        // only start parsing when we see the API start delimiter (0x7E)
        if (serialPort_->peek() == 0x7E) {
            XBeeProcess(xbee_);
        }
    }
}

/**
 * @brief Disconnects the XBee module from the network.
 * @return True if disconnection is successful, otherwise false.
 */
bool XBeeArduino::disconnect() {
    if (xbee_ != nullptr) {
        return XBeeDisconnect(xbee_);
    }
    return false; // Added a return statement here
}

/**
 * @brief Sends data through the XBee module.
 * @param data The data to be sent.
 * @return True if the data is sent successfully, otherwise false.
 */
template <typename T>
bool XBeeArduino::sendData(const T& data) {
    if (moduleType_ == XBEE_LORA) {
        return XBeeSendData(xbee_, &data);
    }
    if (moduleType_ == XBEE_STANDARD) {

        return XBeeSendData(xbee_, &data);
        // // Send data for standard XBee modules
        // Serial.println("Sending data for standard XBee modules");
        // return apiSendFrame(xbee_, data.type, data.data, data.length) == API_SEND_SUCCESS;
    }
    return false;
}

// Explicit template instantiation for xbee_api_frame_t
template bool XBeeArduino::sendData<xbee_api_frame_t>(const xbee_api_frame_t&);

/**
 * @brief Checks if the XBee module is connected to the network.
 * @return True if the module is connected, otherwise false.
 */
bool XBeeArduino::isConnected() {
    if (xbee_ != nullptr) {
        return XBeeConnected(xbee_);
    }
    return false;
}

/**
 * @brief Resets the XBee module.
 */
void XBeeArduino::reset() {
    if (xbee_ != nullptr) {
        return XBeeHardReset(xbee_);
    }
}

/**
 * @brief Sets the API options for XBee.
 * @param options The API options to set.
 * @return True if the options are set successfully, otherwise false.
 */
bool XBeeArduino::setApiOptions(const uint8_t options) {
    if (moduleType_ == XBEE_LORA) {
        return XBeeSetAPIOptions(xbee_, options);
    }
    return false;
}

void XBeeArduino::onReceiveWrapper(XBee* xbee, void* data) {
    if (instance_ && instance_->onReceiveCallback_) {
        instance_->onReceiveCallback_(data); // Forward the call to the original callback
    }
}

void XBeeArduino::onSendWrapper(XBee* xbee, void* data) {
    if (instance_ && instance_->onSendCallback_) {
        instance_->onSendCallback_(data); // Forward the call to the original callback
    }
}

/**
 * @brief Applys config changes on XBee
 * @return True if the changes are applied successfully, otherwise false.
 */
bool XBeeArduino::applyChanges(void) {

    return XBeeApplyChanges(xbee_);
}

/**
 * @brief Write config on XBee
 * @return True if the write config is successfull, otherwise false.
 */
bool XBeeArduino::writeConfig(void) {

    return XBeeWriteConfig(xbee_);
}

uint8_t XBeeArduino::getChecksum(const uint8_t* data, uint16_t len) {
    return calculateChecksum(data, len);
}

bool XBeeArduino::completeTx(const char* data) {
    if (!data) {
        Serial.println("Error: NULL data in completeTx()");
        return false;
    }

    const size_t maxPayloadLen = 100;
    uint8_t asciiBytes[maxPayloadLen] = { 0 };
    int dataLen = asciiToBytes(data, asciiBytes, maxPayloadLen);
    if (dataLen <= 0) {
        Serial.println("Error: asciiToBytes() returned no data.");
        return false;
    }

    // Build the TX Request frame content (only payload, not full frame!)
    uint8_t payload[128];  // enough for 13-byte header + message
    uint8_t* p = payload;

    // Frame ID
    *p++ = 0x01;

    // 64-bit destination address (00 00 00 00 00 00 00 00 for broadcast to coordinator)
    *p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
    *p++ = 0x00; *p++ = 0x00; *p++ = 0xFF; *p++ = 0xFF;

    // 16-bit destination address (FF FF = broadcast)
    *p++ = 0xFF; *p++ = 0xFE;

    // Broadcast radius
    *p++ = 0x00;

    // Transmit options
    *p++ = 0x00;

    // Append your RF data (message)
    memcpy(p, asciiBytes, dataLen);
    p += dataLen;

    uint16_t totalLength = p - payload;

    //  THE FIX: Call apiSendFrame directly
    int status = apiSendFrame(xbee_, XBEE_API_TYPE_TX_REQUEST, payload, totalLength);
    if (status != 0) {
        Serial.println("Error: apiSendFrame failed.");
        return false;
    }

    Serial.println("Frame sent successfully. Raw bytes:");
    for (uint16_t i = 0; i < totalLength; ++i) {
        Serial.printf("0x%02X ", payload[i]);
    }
    Serial.println();

    return true;
}


