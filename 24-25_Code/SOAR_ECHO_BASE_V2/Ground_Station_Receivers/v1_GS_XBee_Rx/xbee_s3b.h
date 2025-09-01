#ifndef XBEE_S3B_H
#define XBEE_S3B_H

#if defined(__cplusplus)
extern "C"
{
#endif

  // Include whatever is needed
#include "xbee.h"

  typedef struct {
    XBee base;  /**< Base class structure. */
    // Add any additional members specific to XBeeS3B
  } XBeeS3B;

  // Function prototypes for the subclass

  XBeeS3B* XBeeS3BCreate(const XBeeCTable* cTable, const XBeeHTable* hTable);
  static void XBeeS3BDestroy(XBeeS3B* self);
  static bool XBeeS3BInit(XBee* self, uint32_t baudrate, void* device);
  static bool XBeeS3BConnect(XBee* self);
  static bool XBeeS3BDisconnect(XBee* self);
  static uint8_t XBeeS3BSendData(XBee* self, const void* data);
  static uint8_t XBeeS3BProcess(XBee* self);
  static bool XBeeS3BConnected(XBee* self);
  static void XBeeS3BHandleRxPacket(XBee* self, void* param);
  static void XBeeS3BHandleTransmitStatus(XBee* self, void* param);
  static void XBeeS3BHandle3RFRxPacket(XBee* self, void* param);


#if defined(__cplusplus)
}
#endif

#endif // XBEE_S3B_H