#ifndef SPH0645_MIC_H
#define SPH0645_MIC_H

#include <driver/i2s_std.h>  // Use the ESP32 I2S standard driver
#include <math.h>            // For log10() function

class SPH0645_Mic {
public:
  // Constructor
  SPH0645_Mic(int wsPin, int sdPin, int sckPin);

  // Initialize the I2S microphone
  void begin();

  // Read audio data into a buffer
  size_t read(int32_t* buffer, size_t bufferSize);

  // Calculate the estimated decibel level (dB SPL) from the audio data
  float getDecibels(size_t numSamples = 1024);

  // Sensitivity Offset: Defines the dB SPL level corresponding to 
  // 0 dBFS (digital full scale). Adjust this value based on 
  // empirical testing. Common values are around 115-125 for MEMS mics.
  // Assumes -64 dBFS @ 46 dB SPL => 46 - (-64) = 110 dB offset.
  static constexpr float DBFS_TO_dBSPL_OFFSET = 110.0;

  bool isReady();
  bool ready = false;

private:
  int wsPin;   // LRCL (Word Select)
  int sdPin;   // DOUT (Data)
  int sckPin;  // BCLK (Bit Clock)
  i2s_chan_handle_t rx_handle;
};

#endif