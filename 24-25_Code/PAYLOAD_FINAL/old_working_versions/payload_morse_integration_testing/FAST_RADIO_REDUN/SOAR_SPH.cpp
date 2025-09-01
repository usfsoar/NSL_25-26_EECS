#include "SOAR_SPH.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>  // Include FreeRTOS for portMAX_DELAY
#include <freertos/task.h>
#include <numeric>  // Required for std::accumulate
#include <vector>   // Required for std::vector
#include <cmath>    // Required for std::sqrt, std::log10, std::abs

// Constructor
SPH0645_Mic::SPH0645_Mic(int wsPin, int sdPin, int sckPin)
  : wsPin(wsPin), sdPin(sdPin), sckPin(sckPin) {}

// Initialize the I2S microphone
void SPH0645_Mic::begin() {
  // I2S channel configuration
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));  // ESP_ERROR_CHECK for debugging

  // I2S standard configuration
  i2s_std_config_t std_cfg = {
    // Sampling rate - 44100 might be high, 16000 or 32000 is often sufficient for SPL
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),  // Lowered sample rate
    // SPH0645 is Mono, 32-bit data width might be okay if left-justified, but 24 or 18 might be more accurate if known
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),  // Set to MONO
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)sckPin,
      .ws = (gpio_num_t)wsPin,
      .dout = I2S_GPIO_UNUSED,
      .din = (gpio_num_t)sdPin,
      .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv = false,
      },
    },
  };
  // Adjust buffer size if needed based on sample rate and processing time

  // Initialize the I2S channel
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));  // Added ESP_ERROR_CHECK
  ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));                   // Added ESP_ERROR_CHECK

  Serial.println("SPH0645 I2S microphone initialized (Mono, 16kHz).");
}

// Read audio data into a buffer
size_t SPH0645_Mic::read(int32_t* buffer, size_t bufferSize) {
  size_t bytesRead = 0;
  esp_err_t err = i2s_channel_read(rx_handle, buffer, bufferSize * sizeof(int32_t), &bytesRead, pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    Serial.printf("I2S Read Error: %s\n", esp_err_to_name(err));
    return 0;  // Return 0 samples on error
  }
  if (bytesRead == 0) {
    Serial.println("I2S Read Timeout or No Data");
  }
  return bytesRead / sizeof(int32_t);  // Return the number of samples read
}

// Calculate the estimated dB SPL level (Revised "From Scratch" Approach)
float SPH0645_Mic::getDecibels(size_t numSamples) {
  if (numSamples == 0) return -100.0;

  // 1. Read Raw Samples into a temporary buffer
  std::vector<int32_t> rawBuffer(numSamples);
  size_t samplesRead = read(rawBuffer.data(), numSamples);

  if (samplesRead == 0) {
    Serial.println("No samples read.");
    return -100.0;
  }

  // 2. Filter invalid samples and Extract Effective Bits (Assume 24-bit left-justified)
  std::vector<int32_t> validSamples24bit;
  validSamples24bit.reserve(samplesRead / 2);  // Pre-allocate roughly half the size
  const int32_t INVALID_SAMPLE_THRESHOLD = 100;

  for (size_t i = 0; i < samplesRead; ++i) {
    if (std::abs(rawBuffer[i]) > INVALID_SAMPLE_THRESHOLD) {
      // Right-shift to get the 24-bit signed value
      validSamples24bit.push_back(rawBuffer[i] >> 8);
    }
  }

  size_t validSampleCount = validSamples24bit.size();
  if (validSampleCount < 10) {  // Need a minimum number of samples for meaningful analysis
    Serial.printf("Insufficient valid samples after filtering: %u\n", validSampleCount);
    return -100.0;
  }

  // 3. Remove DC Offset
  // Calculate the average (DC offset) of the valid 24-bit samples
  double sum = 0.0;
  for (int32_t sample : validSamples24bit) {
    sum += sample;
  }
  double dcOffset = sum / validSampleCount;

  // Subtract DC offset and calculate sum of squares of the AC component
  double sumOfSquaresAC = 0.0;
  for (int32_t sample : validSamples24bit) {
    double acSample = (double)sample - dcOffset;
    sumOfSquaresAC += acSample * acSample;
  }

  // 4. Calculate RMS of AC Component
  double meanSquareAC = sumOfSquaresAC / validSampleCount;
  double rmsAC = std::sqrt(meanSquareAC);

  // Handle silence or very low signal post-DC removal
  const double RMS_SILENCE_THRESHOLD = 1e-3;  // Adjust threshold based on noise floor
  if (rmsAC < RMS_SILENCE_THRESHOLD) {
    Serial.println("RMS below silence threshold after DC removal.");
    return -100.0; 
  }

  // 5. Convert RMS to dBFS (relative to 24-bit full scale)
  // Max positive 24-bit value is 2^23 - 1 = 8388607.0
  const double MAX_24BIT_VALUE = 8388607.0;
  double dbfs = 20.0 * std::log10(rmsAC / MAX_24BIT_VALUE);

  // 6. Convert dBFS to dB SPL
  // Use the offset determined experimentally. Start with 62.0 again,
  // as the RMS calculation base has changed significantly.
  const float dBFS_TO_dBSPL_OFFSET_EXPERIMENTAL = 110.0;  // -64 dBFS @ 46 dB SPL -> Offset 110
  float dbSpl = dbfs + dBFS_TO_dBSPL_OFFSET_EXPERIMENTAL;

  // Optional: Clamp output range
  // if (dbSpl < 20) dbSpl = 20; // Set a floor value
  // if (dbSpl > 125) dbSpl = 125; // Set a ceiling

  // Debugging
  // Serial.printf("Samples: %u, DC Offset: %.1f, RMS(AC): %.1f, dBFS: %.2f, dB SPL: %.2f\n",
  //               validSampleCount, dcOffset, rmsAC, dbfs, dbSpl);


  return dbSpl;
}