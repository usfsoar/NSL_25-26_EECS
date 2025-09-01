#include "xbee_S3B.h"
#include "xbee_api_frames.h"
#include "xbee.h"
#include <stdlib.h>
#include <FreeRTOS.h>



// Define the VTable for the S3B XBee module
static const XBeeVTable XBeeS3B_VTable = {
    .init = XBeeS3BInit,
    .connect = XBeeS3BConnect,
    .disconnect = XBeeS3BDisconnect,
    .sendData = XBeeS3BSendData,
    // .softReset = XBeeSoftReset,
    // .hardReset = XBeeHardReset,
    .process = XBeeS3BProcess,
    .connected = XBeeS3BConnected,
    .handleRxPacketFrame = XBeeS3BHandleRxPacket,
    .handleTransmitStatusFrame = XBeeS3BHandleTransmitStatus,
    .handle3RFRxPacketFrame = XBeeS3BHandle3RFRxPacket,
};

// Implement the create and destroy methods
XBeeS3B* XBeeS3BCreate(const XBeeCTable* cTable, const XBeeHTable* hTable) {
    XBeeS3B* instance = (XBeeS3B*)malloc(sizeof(XBeeS3B));
    instance->base.vtable = &XBeeS3B_VTable;
    instance->base.htable = hTable;
    instance->base.ctable = cTable;
    return instance;
}

void XBeeS3BDestroy(XBeeS3B* self) {
    free(self);
}

// DO NOT CALL THIS FUNCTIOn to setup a serial port - just define serial yourself in the .ino file
static bool XBeeS3BInit(XBee* self, uint32_t baudrate, void* device) {
    // Logic is not utilized
    // // Implement initialization logic specific to XBeeS3B
    // XBEEDebugPrint("In the initializing function\n");
    // return (self->htable->PortUartInit(baudrate, device) == UART_SUCCESS) ? true : false;
    return true;
}

static bool XBeeS3BConnect(XBee* self) {
    // Implement connection logic specific to XBeeS3B

    // uint32_t startTime = self->htable->PortMillis();
    // while ((self->htable->PortMillis() - startTime) < 12000) {
    //     if (XBeeS3BConnected(self)) {
    //         XBEEDebugPrint("Successfully connected\n");
    //         return true;
    //     }
    //     self->htable->PortDelay(500);
    // }
    // XBEEDebugPrint("Failed to connect\n");
    // return false;
    return true;
}

static bool XBeeS3BDisconnect(XBee* self) {
    // Implement disconnection logic specific to XBeeS3B
    return true;
}

static uint8_t XBeeS3BSendData(XBee* self, const void* data) {
    // Cast the provided data pointer to an xbee_api_frame_t pointer.
    xbee_api_frame_t* frame = (xbee_api_frame_t*)data;
    // Call apiSendFrame() with the frame's type, data pointer, and length.

    int send_status = apiSendFrame(self, frame->type, frame->data, frame->length);
    if (send_status != API_SEND_SUCCESS) {
        // XBEEDebugPrint("In xbee_s3b.c XBeeS3BSendData: Failed to send the frame\n");
        return 0xFF;  // Failed to send the frame
    }
    // XBEEDebugPrint("In xbee_s3b.c XBeeS3BSendData: Frame sent successfully\n");
    // return API_SEND_SUCCESS;    // Ensure we return here on success

    // uint32_t startTime = self->htable->PortMillis();
    // while ((self->htable->PortMillis() - startTime) < 5000) {
    //     if (self->txStatusReceived) {
    //         // self->txStatusReceived = false;
    //         // XBEEDebugPrint(self);
    //         if (self->deliveryStatus) {
    //             XBEEDebugPrint("In xbee_s3b.c XBeeS3BSendData: TX Delivery Status 0x%02X\n", self->deliveryStatus);
    //             XBeeS3BProcess(self);
    //         }
    //         return self->deliveryStatus;
    //     }
    //     self->htable->PortDelay(10);
    // }
    return XBeeS3BProcess(self); // Call the process function to handle incoming frames

    XBEEDebugPrint("Failed to receive TX Request Status frame\n");
    return 0xFF;  // Indicate failure or timeout
}

static uint8_t XBeeS3BProcess(XBee* self) {

    uint32_t startTime = xTaskGetTickCount();
    // while ((xTaskGetTickCount() - startTime) < pdMS_TO_TICKS(1000)) {
    if (self->txStatusReceived) {
        // self->txStatusReceived = false;
        // XBEEDebugPrint(self);
        if (self->deliveryStatus) {
            // XBEEDebugPrint("In xbee_s3b.c ...: TX Delivery Status 0x%02X\n", self->deliveryStatus);

            // XBeeS3BProcess(self) code begins ---

            xbee_api_frame_t frame;
            // XBEEDebugPrint("In xbee_s3b.c XBeeS3BProcess: Processing the frame\n");

            // self->htable->PortDelay(300);
            self->htable->PortFlushRx(); // Flush the UART buffer before receiving
            vTaskDelay(pdMS_TO_TICKS(50)); // Add a delay to allow for data to be received
            int status = apiReceiveApiFrame(self, &frame);
            // XBEEDebugPrint("This is the status: %d\n", status);
            if (status == API_RECEIVE_SUCCESS || status == API_SEND_SUCCESS) {
                // self->htable->PortDelay(500);
                apiHandleFrame(self, frame);
            }
            self->htable->PortFlushRx();

            // else if (status != API_RECEIVE_ERROR_TIMEOUT_START_DELIMITER) {
            // XBEEDebugPrint("In xbee_s3b.c XBeeS3BProcess: Error receiving frame, status: %d\n", status);
            // }
            // else {
            // XBEEDebugPrint("In xbee_s3b.c XBeeS3BProcess: Frame status: 0x%08X\n", (unsigned int)status);
            // }

            // XBeeS3BProcess code ends ---


        }
        return self->deliveryStatus;
    }
    // self->htable->PortDelay(10);
// }
// XBEEDebugPrint("Failed to receive TX Request Status frame\n");
    return 0xFF;  // Indicate failure or timeout

}

static bool XBeeS3BConnected(XBee* self) {
    // Flush any residual data before sending AT command
    // self->htable->PortFlushRx();

    // XBEEDebugPrint("Checking connection status\n in XBeeS3BConnected()");
    // uint8_t response;
    // uint8_t responseLength;
    // int status;

    // status = apiSendAtCommandAndGetResponse(self, AT_AP, NULL, 0, &response, &responseLength, 10000);
    // XBEEDebugPrint("Status: %d\n", status);
    // if (status == 0) {
    //     XBEEDebugPrint("Success, AT_ID Resp: %d\n", response);
    //     return true;
    // }
    // else {
    //     XBEEDebugPrint("Failed to receive AT_ID response, error code: %d\n", status);
    //     return false;
    // }
    return true;
}

static void XBeeS3BHandleRxPacket(XBee* self, void* param) {

    // If no input is given, exit
    if (param == NULL) return;
    // APIFrameDebugPrint("In xbee_s3b.c XBeeS3BHandleRxPacket: Handling RX packet\n");
    // Cast the provided parameter to an xbee_api_frame_t pointer
    xbee_api_frame_t* frame = (xbee_api_frame_t*)param;
    // Check if the frame type is either RX_PACKET or EXPLICIT_RX_PACKET or else exit
    if (frame->type != XBEE_API_TYPE_3RF_RX_PACKET && frame->type != XBEE_API_TYPE_3RF_RX_EXPLICIT_PACKET) return;

    // Calculate the checksum
    // // First obtain the checksum of the frame itself
    // uint8_t checksum = calculateChecksum(frame->data, frame->length - 1);
    // // Read the checksum from the last byte of the frame
    // uint8_t received_checksum = frame->data[frame->length - 1];
    // // Compare the two checksums
    // if (checksum != received_checksum) {
    //     APIFrameDebugPrint("In xbee_s3b.c XBeeS3BHandleRxPacket: Checksum mismatch, received frame: ");
    //     for (int i = 0; i < frame->length; i++) {
    //         APIFrameDebugPrint("0x%02X ", frame->data[i]);
    //     }
    //     APIFrameDebugPrint("\n");
    //     APIFrameDebugPrint("Checksum mismatch: calculated 0x%02X, received 0x%02X\n", checksum, received_checksum);
    //     return; // Exit if the checksums do not match
    // }


    // Send to the callback function
    if (self->ctable->OnReceiveCallback) {
        self->ctable->OnReceiveCallback(self, frame); // Pass the address of the stack variable
    }


}

static void XBeeS3BHandleTransmitStatus(XBee* self, void* param) {
    xbee_api_frame_t* frame = (xbee_api_frame_t*)param;
    if (frame->type != XBEE_API_TYPE_TX_STATUS) return;

    // The first byte of frame->data is the Frame ID
    uint8_t frame_id = frame->data[1];

    // The next byte is the delivery status
    uint8_t delivery_status = frame->data[2];

    // The next two bytes are the destination address
    uint8_t dest_addr_msb = frame->data[3];
    uint8_t dest_addr_lsb = frame->data[4];

    // Print the basic information
    APIFrameDebugPrint("TX Status Response:\n");
    APIFrameDebugPrint("  Frame ID: %d\n", frame_id);
    APIFrameDebugPrint("  Delivery Status: %d\n", delivery_status);
    APIFrameDebugPrint("  Destination Address: 0x%02X%02X\n", dest_addr_msb, dest_addr_lsb);

    // Check if there is additional data in the frame
    if (frame->length > 5) {
        APIFrameDebugPrint("  Data: ");
        // Print the remaining data bytes
        APIFrameDebugPrint("%s\n", &(frame->data[5]));
    }
    else {
        APIFrameDebugPrint("  No additional data.\n");
    }

}

static void XBeeS3BHandle3RFRxPacket(XBee* self, void* param) {
    // Implement logic to handle received 3RF packets specific to XBeeNew

}



