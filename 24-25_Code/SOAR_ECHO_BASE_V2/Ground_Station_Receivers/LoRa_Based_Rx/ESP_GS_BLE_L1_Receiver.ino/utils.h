#ifndef UTILS_H
#define UTILS_H

class Utils{
    public:
        static uint32_t bytesToUint32(const byte bytes[4]){
            uint32_t value = 0;
            value |= ((uint32_t)bytes[0] << 24);
            value |= ((uint32_t)bytes[1] << 16);
            value |= ((uint32_t)bytes[2] << 8);
            value |= ((uint32_t)bytes[3]);
            return value;
        }
        static void floatToBytes(float value, byte* byteArray) {
            memcpy(byteArray, &value, sizeof(float));
        } 

};






#endif //UTILS_H