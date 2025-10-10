#include <SPI.h>
#include <RH_RF95.h>

// Pin definitions for Arduino Uno
#define RFM95_CS  4
#define RFM95_RST 3
#define RFM95_INT 2   // DIO0

// Must match the transmitter
#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(9600);
  while (!Serial); // Wait for Serial

  Serial.println("Arduino Uno LoRa RX Test!");

  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Boost receive sensitivity (default is fine, but this helps in noisy environments)
  rf95.setTxPower(23, false);
}

void loop()
{
  if (rf95.available())
  {
    // Buffer for incoming message
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      Serial.print("Got message: ");
      Serial.println((char*)buf);

      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // Send a reply back
      uint8_t reply[] = "Ack";
      rf95.send(reply, sizeof(reply));
      rf95.waitPacketSent();
      Serial.println("Sent reply: Ack");
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}


// #include <SPI.h>
// #include <RH_RF95.h>
// // #include "..\\LoRa_Protocol\\LoRaProtocol.h"

// // Pin definitions for Arduino Uno
// #define RFM95_CS  10
// #define RFM95_RST 9
// #define RFM95_INT 2   // DIO0

// // Must match the transmitter
// #define RF95_FREQ 433.0

// RH_RF95 rf95(RFM95_CS, RFM95_INT);

// void setup() 
// {
//   pinMode(RFM95_RST, OUTPUT);
//   digitalWrite(RFM95_RST, HIGH);

//   Serial.begin(9600);
//   while (!Serial); // Wait for Serial

//   Serial.println("Arduino Uno LoRa RX Test!");

//   // Manual reset
//   digitalWrite(RFM95_RST, LOW);
//   delay(10);
//   digitalWrite(RFM95_RST, HIGH);
//   delay(10);

//   if (!rf95.init()) {
//     Serial.println("LoRa radio init failed");
//     while (1);
//   }
//   Serial.println("LoRa radio init OK!");

//   if (!rf95.setFrequency(RF95_FREQ)) {
//     Serial.println("setFrequency failed");
//     while (1);
//   }
//   Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

//   // Boost receive sensitivity (default is fine, but this helps in noisy environments)
//   rf95.setTxPower(23, false);
// }

// void loop()
// {
//   if (rf95.available())
//   {
//     // Buffer for incoming message (leave room for NUL so we can safely print as string)
//     uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
//     uint8_t len = RH_RF95_MAX_MESSAGE_LEN; // capacity without NUL

//     if (rf95.recv(buf, &len))
//     {
//   // Unpack header and data
//   char data[RH_RF95_MAX_MESSAGE_LEN + 1];
//   DataType dtype = TYPE_GENERIC;
//   unpackData(buf, len, &dtype, data, sizeof(data));

//         // Map types to V3 filenames (conceptually)
//         const char* fileForType = "";
//         switch (dtype) {
//           case TYPE_IMU: fileForType = "/imu.csv"; break;
//           case TYPE_ALTIMETER: fileForType = "/altimeter.csv"; break;
//           case TYPE_GPS: fileForType = "/gps.csv"; break;
//           default: fileForType = "/generic.txt"; break;
//         }

//         Serial.print("Got message (type "); Serial.print((int)dtype); Serial.print(") -> "); Serial.println(fileForType);
//   Serial.println(data);

//         Serial.print("RSSI: ");
//         Serial.println(rf95.lastRssi(), DEC);

//         // Send a reply back (send only the visible characters, no trailing NUL)
//         const char replyStr[] = "Ack";
//         rf95.send((uint8_t*)replyStr, (uint8_t)strlen(replyStr));
//         rf95.waitPacketSent();
//         Serial.println("Sent reply: Ack");
//     }
//     else
//     {
//       Serial.println("Receive failed");
//     }
//   }
// }