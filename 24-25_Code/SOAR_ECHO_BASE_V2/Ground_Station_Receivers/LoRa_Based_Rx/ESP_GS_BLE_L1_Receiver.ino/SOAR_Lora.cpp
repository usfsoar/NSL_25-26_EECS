#include "SOAR_Lora.h"

SOAR_Lora::SOAR_Lora(String my_address, String network_id, String frequency, unsigned long queueTime) :
    address(my_address),
    MIN_QUEUE_TIME(queueTime),
    network_id(network_id),
    frequency(frequency) {}


void SOAR_Lora::begin(int chosenSerial) {
    //If it's arduino mega, use Serial1
    //Otherwise use HardwareSerial(0), and do begin(115200, SERIAL_8N1, -1, -1);
    #if defined(__AVR_ATmega2560__)
        switch (chosenSerial){
          case 1:
            loraSerial = &Serial1;
            break;
          case 2:
            loraSerial = &Serial2;
            break;
          case 3:
            loraSerial = &Serial3;
            break;
          default:
            Serial.println("Invalid Serial.");
            return;  
        }
        loraSerial->begin(115200);
    #elif defined(__IMXRT1062__)
        switch (chosenSerial){
          case 1:
            loraSerial = &Serial1;
            break;
          case 2:
            loraSerial = &Serial2;
            break;
          case 3:
            loraSerial = &Serial3;
            break;
          case 4:
            loraSerial = &Serial4;
            break;
          case 5:
            loraSerial = &Serial5;
            break;  
          default:
            Serial.println("Invalid Serial.");
            return;  
        }
        loraSerial->begin(115200);
    #else
        loraSerial = new HardwareSerial(0);  // Assuming LoRa is connected to Serial0
        loraSerial->begin(115200, SERIAL_8N1, -1, -1);
    #endif
    loraSendStr(("AT+ADDRESS=" + address).c_str());
    loraSendStr(("AT+BAND=" + frequency).c_str());
    loraSendStr(("AT+NETWORKID=" + network_id).c_str());
    lastSentTime = 0;
}

bool SOAR_Lora::available() {
    return loraSerial->available();
}
// String SOAR_Lora::read() {
//     Serial.print("Request Received: ");
//     String incomingString = loraSerial->readString();
//     delay(50);
//     char dataArray[incomingString.length()];
//     incomingString.toCharArray(dataArray, incomingString.length());
//     char* data = strtok(dataArray, ",");
//     data = strtok(NULL, ",");
//     data = strtok(NULL, ",");
//     Serial.println(data);
//     String data_str = String(data);
//     return data_str;
// }

bool SOAR_Lora::read(int *address, int *length, byte **data, int *rssi, int *snr) {
    *length = 0; // Default length to 0
    if(!loraSerial->available()) {
        return false;
    }
    unsigned long startTime = millis();
    const unsigned long TIMEOUT = 1000; // Timeout in milliseconds
    while (millis() - startTime < TIMEOUT) {
        if (loraSerial->available()) {
            // Look for the start of a valid message
            if (loraSerial->peek() == '+') {
                // Set a timeout for the loraSerial
                String response = loraSerial->readStringUntil(','); // Read up to the first comma
                if (response.startsWith("+RCV=")) {
                    response.remove(0, 5); // Remove "+RCV="
                    *address = response.toInt(); // Read the address
                    String lengthStr = loraSerial->readStringUntil(','); // Read up to the next comma, it discards the comma
                    *length = lengthStr.toInt(); // Read the length
                    if (*length > 0) {
                        *data = new byte[*length];
                        int bytesRead = 0;
                        unsigned long dataStartTime = millis();
                        const unsigned long DATA_TIMEOUT = 1000; // Timeout in milliseconds for reading data

                        while (millis() - dataStartTime < DATA_TIMEOUT && bytesRead < *length) {
                            if (loraSerial->available()) {
                                (*data)[bytesRead] = (byte)loraSerial->read();
                                bytesRead++;
                            }
                        }

                        if (bytesRead < *length) {
                            Serial.println("Not enough data available within timeout");
                            // Not enough data available within timeout, clean up and return false
                            // delete[] *data;
                            // *data = nullptr;
                            *length = bytesRead;
                            return false;
                        }
                        
                        String rssiStr = loraSerial->readStringUntil(','); // Read up to the next comma, it discards the comma
                        *rssi = rssiStr.toInt(); // Read the RSSI
                        String snrStr = loraSerial->readStringUntil('\n'); // Read up to the end of the message
                        *snr = snrStr.toInt(); // Read the SNR
                        return true; // Successfully read the message
                    }
                }
            } else {
                //Read and discard the character
                loraSerial->read();
            }
        }
    }
    return false; // Timeout reached or no valid "+RCV" message
}

//Given an array of bytes, assume last 2 bytes are the checksum, and return true if the checksum is correct
bool SOAR_Lora::checkChecksum(byte* data, int length) {
    if (length < 2) {
        return false;
    }

    // Extract the checksum from the last 2 bytes
    uint16_t checksum = (uint16_t)((data[length - 2] << 8) | data[length - 1]);

    // Calculate the checksum of the received data (excluding the checksum bytes)
    uint16_t calculatedChecksum = 0;
    for (int i = 0; i < length - 2; i++) {
        calculatedChecksum += data[i];
    }

    Serial.println("Checksum: " + String(checksum) + " Calculated: " + String(calculatedChecksum));

    // Compare the calculated checksum with the received checksum
    return calculatedChecksum == checksum;
}



//Given an array of bytes, it's length, and an array of chars, and a start index, see if the bytes match the chars


//Given a byte array, it's length, and a starti index return the bytes as float
bool SOAR_Lora::bytesToFloat(byte* data, int start , float* value) {
    if (start + sizeof(float) > sizeof(data)) {
        return false;
    }
    union {
        float f;
        byte b[sizeof(float)];
    } u;
    for (int i = 0; i < static_cast<int>(sizeof(float)); i++) {
        u.b[i] = data[start + i];
    }
    *value = u.f;
    return true;
}
bool SOAR_Lora::bytesToInt(byte* data, int start, int* value) {
    if (start + sizeof(int) > sizeof(data)) {
        return false;
    }
    union {
        int i;
        byte b[sizeof(int)];
    } u;
    for (int i = 0; i < static_cast<int>(sizeof(int)); i++) {
        u.b[i] = data[start + i];
    }
    *value = u.i;
    return true;
}


void SOAR_Lora::beginPacket() {
    bufferIndex = 0; // Reset buffer index
    checksum = 0; // Reset checksum
}

void SOAR_Lora::sendChar(const char *str) {
    int limit = 1000;
    int i = 0;
    while (*str && i < limit) {
        packetBuffer[bufferIndex++] = *str;
        checksum += *str;
        str++;
        i++;
    }
}
void SOAR_Lora::sendByte(byte value) {
    packetBuffer[bufferIndex++] = value;
    checksum += value;
}
void SOAR_Lora::sendBytes(const byte* value, int length) {
    for (int i = 0; i < length; i++) {
        packetBuffer[bufferIndex++] = value[i];
        checksum += value[i];
    }
}

void SOAR_Lora::sendFloat(float value) {
    byte *bytes = (byte *)&value;
    for (size_t i = 0; i < sizeof(float); ++i) {
        packetBuffer[bufferIndex++] = bytes[i];
        checksum += bytes[i];
    }
}

void SOAR_Lora::sendInt(int value) {
    byte *bytes = (byte *)&value;
    for (size_t i = 0; i < sizeof(int); ++i) {
        packetBuffer[bufferIndex++] = bytes[i];
        checksum += bytes[i];
    }
}
void SOAR_Lora::sendUInt8(uint8_t value){
    byte *bytes = (byte *)&value;
    for (size_t i = 0; i < sizeof(uint8_t); ++i) {
        packetBuffer[bufferIndex++] = bytes[i];
        checksum += bytes[i];
    }
}

void SOAR_Lora::sendLong(uint32_t value) {
    byte *bytes = (byte *)&value;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        packetBuffer[bufferIndex++] = bytes[i];
        checksum += bytes[i];
    }
}

void SOAR_Lora::endPacket(int address) {
    //Add to bytes to the packet buffer for the checksum
    //uint16_t checksum = 0;
    //for (size_t i = 0; i < bufferIndex; ++i) {
    //    checksum += packetBuffer[i];
    //}
    // Split checksum into 2 bytes and add to the packet buffer
    packetBuffer[bufferIndex++] = (checksum >> 8) & 0xFF; // High byte
    packetBuffer[bufferIndex++] = checksum & 0xFF;    
    Packet packet;
    packet.address = address;
    packet.data = new byte[bufferIndex];
    memcpy(packet.data, packetBuffer, bufferIndex);
    packet.length = bufferIndex;
    messageQueue.push(packet);
    bufferIndex = 0; // Reset packet length for the next packet
    checksum = 0; // Reset checksum for the next packet
    handleQueue(); // Attempt to send the packet immediately
}
void SOAR_Lora::endPacketWTime(int address){
    this->sendLong(millis());
    this->endPacket(address);
}
void SOAR_Lora::stringPacketWTime(const char* str, int address) {
    //begin, send the string, end
    beginPacket();
    sendChar(str);
    sendLong(millis());
    endPacket(address);
}
void SOAR_Lora::stringPacket(const char* str, int address) {
    //begin, send the string, end
    beginPacket();
    sendChar(str);
    endPacket(address);
}

void SOAR_Lora::handleQueue() {
    if (!messageQueue.empty() && (millis() - lastSentTime >= MIN_QUEUE_TIME)) {
        Packet packet = messageQueue.front();
        int address = packet.address;
        int packetLength = packet.length;
        String header = "AT+SEND=" + String(address) + "," + String(packetLength) + ",";
        byte* newPacket = new byte[packet.length + header.length()];
        for (int i = 0; i < static_cast<int>(header.length()); i++) {
          newPacket[i] = header[i];
        }
        int headerLen = header.length();
        for (int i = 0; i < packet.length; i++) {
            newPacket[i + headerLen] = packet.data[i];
        }
        loraSend(newPacket, packet.length + header.length());
        delete[] newPacket; // Free the allocated memory for the packet data
        messageQueue.pop();
        lastSentTime = millis();
    }
}


String SOAR_Lora::sendATCommand(const byte* command, int length, unsigned long timeout) {
    String result;
    Serial.print("Sending: ");
    for (int i = 0; i < length; ++i) {
        // Serial.write(command[i]);
        //print the bytes as characters
        Serial.print((char)command[i]);
    }
    Serial.println();
    loraSerial->write(command, length);
    loraSerial->println();  // Ensure the command is terminated

    unsigned long startTime = millis();
    uint32_t last_read_time = 0;
    Serial.print("Received: ");
    while (millis() - startTime < timeout) { // Wait for a response
        if (loraSerial->available()) {
            char c = loraSerial->read();
            Serial.write(c);
            result += c;  // append to the result string
            last_read_time = millis();
        }
        if(last_read_time != 0 && millis() - last_read_time > 100) {
            break; // A response came in, but it's been too long since the last character
        }
    }
    Serial.println();  // new line after timeout.
    return result;
}
void SOAR_Lora::loraSendStr(const char* toSend, unsigned long timeout) {
    loraSend((const byte*)toSend, strlen(toSend), timeout);
}

void SOAR_Lora::loraSend(const byte* toSend, int length, unsigned long timeout) {

  #if !DIGITAL_TWIN
    for (int i = 0; i < 3; i++) {
        String response = sendATCommand(toSend, length, timeout);
        if (response.indexOf("+OK") >= 0) {
            break;
        } else if (response.indexOf("+ERR") >= 0) {
            Serial.println("Error response detected. Retrying...");
        } 
    }
    
  #else
    //Send lora message as bytes on the regular serial port
    //Request header 0x01 followed by 0x02, followed by the length of the message, followed by the message
    int len = strlen(toSend);
    Serial.write(0x10);
    Serial.write(0x02);
    Serial.write(len);
    //Send all the message bytes
    for (int i = 0; i < len; i++) {
        Serial.write(toSend[i]);
    }
  #endif
}

