#ifndef EFFECT_COMPRESSOR_HPP
#define EFFECT_COMPRESSOR_HPP

#include <algorithm>
#include <cmath>

/**
 * @brief A simple audio compressor class.
 */
class Compressor {
 public:
  /**
   * @brief Constructor for Compressor.
   *
   * @param threshold Threshold in dB.
   * @param ratio Compression ratio (e.g., 4.0 for 4:1).
   * @param attack Attack time in milliseconds.
   * @param release Release time in milliseconds.
   * @param makeupGain Makeup gain in dB.
   * @param sampleRate Sampling rate in Hz.
   */
  Compressor(float threshold = -20.0f, float ratio = 4.0f, float attack = 5.0f,
             float release = 50.0f, float makeupGain = 0.0f,
             float sampleRate = 44100.0f);

  /**
   * @brief Processes a buffer of audio samples.
   *
   * @param buffer Pointer to the audio buffer.
   * @param numSamples Number of samples in the buffer.
   */
  void process(float* buffer, int numSamples);

  // Setters for parameters
  void setThreshold(float threshold);
  void setRatio(float ratio);
  void setAttack(float attack);
  void setRelease(float release);
  void setMakeupGain(float makeupGain);

 private:
  // Parameters
  float m_thresholdDb;
  float m_ratio;
  float m_attackMs;
  float m_releaseMs;
  float m_makeupGainDb;

  // Internal state
  float m_envelope;
  float m_sampleRate;

  // Helper functions
  float dbToLinear(float db);
  float linearToDb(float linear);
};

#endif  // EFFECT_COMPRESSOR_HPP
