/**
 * @file xbee.c
 * @brief Implementation of the XBee class.
 *
 * This file contains the implementation of the core functions used to interact
 * with XBee modules using the API frame format. The functions provide an interface
 * for initializing the module, connecting/disconnect to the nework,
 * sending and receiving data, and handling AT commands.
 *
 * @version 1.0
 * @date 2024-08-08
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
 *
 * @author Felix Galindo
 * @contact felix.galindo@digi.com
**/

#include "xbee.h"
#include "xbee_api_frames.h" 
#include "serial_helper.h"

// Base class methods

/**
 * @brief Initializes the XBee module.
 *
 * This function initializes the XBee module by setting the initial frame ID counter
 * and calling the XBee subclass specific initialization routine.
 *
 * @param[in] self Pointer to the XBee instance.
 * @param[in] baudrate Baud rate for the serial communication.
 * @param[in] device Path to the serial device (e.g., "/dev/ttyUSB0").
 *
 * @return True if initialization is successful, otherwise false.
 */
 // DO NOT CALL THIS FUNCTIOn to setup a serial port - just define serial yourself in the .ino file
bool XBeeInit(XBee* self, uint32_t baudRate, void* device) {
    self->frameIdCntr = 1;
    XBEEDebugPrint("In XBeeInit of xbee.c, baudrate is: %d\n", baudRate);
    return self->vtable->init(self, baudRate, device);
}

/**
 * @brief Connects the XBee to the network.
 *
 * This function connects to the network by calling the XBee subclass specific
 * connection implementation provided by the XBee subclass. This is a blocking function.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return True if the connection is successful, otherwise false.
 */
bool XBeeConnect(XBee* self) {
    return self->vtable->connect(self);
}

/**
 * @brief Disconnects the XBee from the network.
 *
 * This function closes the connection by calling the platform-specific
 * close implementation provided by the XBee subclass. This is a blocking function.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return True if the disconnection is successful, otherwise false.
 */
bool XBeeDisconnect(XBee* self) {
    return self->vtable->disconnect(self);
}

/**
 * @brief Request XBee to send data over network.
 *
 * This function sends data over the network by calling the XBee subclass specific
 * send data implementation provided by the XBee subclass. This is a blocking function.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return xbee_deliveryStatus_t, 0 if successful
 */
uint8_t XBeeSendData(XBee* self, const void* data) {
    return self->vtable->sendData(self, data);
}

/**
 * @brief Performs a soft reset of the XBee module.
 *
 * This function invokes the `soft_reset` method defined in the XBee subclass's
 * virtual table (vtable) to perform a soft reset of the XBee module. A soft reset
 * typically involves resetting the module's state without a full power cycle.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return bool Returns true if the soft reset is successful, otherwise false.
 */
bool XBeeSoftReset(XBee* self) {
    return self->vtable->softReset(self);
}

/**
 * @brief Performs a hard reset of the XBee module.
 *
 * This function invokes the `hard_reset` method defined in the XBee subclass's
 * virtual table (vtable) to perform a hard reset of the XBee module. A hard reset
 * usually involves a full power cycle or reset through rst pin, resetting the module completely.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return void This function does not return a value.
 */
void XBeeHardReset(XBee* self) {
    self->vtable->hardReset(self);
}

/**
 * @brief Calls the XBee subclass's process implementation.
 *
 * This function invokes the `process` method defined in the XBee subclass's
 * virtual table (vtable). It is responsible for processing any ongoing tasks
 * or events related to the XBee module and must be called continuously in the
 * application's main loop to ensure proper operation.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return void This function does not return a value.
 */
void XBeeProcess(XBee* self) {
    self->vtable->process(self);
}

/**
 * @brief Checks if the XBee module is connected to the network.
 *
 * This function calls the `connected` method defined in the XBee subclass's
 * virtual table (vtable) to determine if the XBee module is currently connected
 * to the network. It returns true if the module is connected, otherwise false.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return bool Returns true if the XBee module is connected, otherwise false.
 */
bool XBeeConnected(XBee* self) {
    return self->vtable->connected(self);  // Only call if all checks passed
}



/**
 * @brief Sends the ATWR command to write the current configuration to the XBee module's non-volatile memory.
 *
 * This function sends the ATWR command using an API frame to write the current configuration settings
 * to the XBee module's non-volatile memory. The function waits for a response from the module
 * to confirm that the command was successful. If the command fails or the module does not respond,
 * a debug message is printed.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return bool Returns true if the configuration was successfully written, otherwise false.
 */
bool XBeeWriteConfig(XBee* self) {
    uint8_t response[33];
    uint8_t responseLength;
    int status = apiSendAtCommandAndGetResponse(self, AT_WR, NULL, 0, response, &responseLength, 5000);
    if (status != API_SEND_SUCCESS) {
        XBEEDebugPrint("Failed to Write Config\n");
    }
    return status;
}

/**
 * @brief Sends the ATAC command to apply pending configuration changes on the XBee module.
 *
 * This function sends the ATAC command using an API frame to apply any pending configuration changes
 * on the XBee module. The function waits for a response from the module to confirm that the command
 * was successful. If the command fails or the module does not respond, a debug message is printed.
 *
 * @param[in] self Pointer to the XBee instance.
 *
 * @return bool Returns true if the changes were successfully applied, otherwise false.
 */

bool XBeeApplyChanges(XBee* self) {
    uint8_t response[33];
    uint8_t responseLength;
    int status = apiSendAtCommandAndGetResponse(self, AT_AC, NULL, 0, response, &responseLength, 5000);
    if (status != API_SEND_SUCCESS) {
        XBEEDebugPrint("Failed to Apply Changes\n");
    }
    return status;
}

/**
 * @brief Sends the AT_AO command to set API Options.
 *
 * This function configures the API Options on the XBee module
 * by sending the AT command `AT_AO` with the specified API Options value. The function
 * is blocking, meaning it waits for a response from the module or until a timeout
 * occurs. If the command fails to send or the module does not respond, a debug
 * message is printed.
 *
 * @param[in] self Pointer to the XBee instance.
 * @param[in] value The API Options to be set, provided as a string.
 *
 * @return bool Returns true if the API Options was successfully set, otherwise false.
 */
bool XBeeSetAPIOptions(XBee* self, const uint8_t value) {
    uint8_t response[33];
    uint8_t responseLength;
    int status = apiSendAtCommandAndGetResponse(self, AT_AO, (const uint8_t[]) { value }, 1, response, & responseLength, 5000);
    if (status != API_SEND_SUCCESS) {
        XBEEDebugPrint("Failed to set API Options\n");
    }
    return status;
}

