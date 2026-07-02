#ifndef EFFECT_LIMITER_HPP
#define EFFECT_LIMITER_HPP

#include <algorithm>
#include <cmath>

/**
 * @brief A simple audio limiter class.
 *
 * A limiter is a compressor with a very high ratio (typically infinity:1)
 * that prevents the signal from exceeding a set threshold level.
 */
class Limiter {
 public:
  /**
   * @brief Constructor for Limiter.
   *
   * @param threshold Threshold in dB (maximum allowed level).
   * @param attack Attack time in milliseconds.
   * @param release Release time in milliseconds.
   * @param sampleRate Sampling rate in Hz.
   */
  Limiter(float threshold = -3.0f, float attack = 1.0f, float release = 10.0f,
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
  void setAttack(float attack);
  void setRelease(float release);

 private:
  // Parameters
  float m_thresholdDb;
  float m_attackMs;
  float m_releaseMs;

  // Internal state
  float m_envelope;
  float m_sampleRate;

  // Helper functions
  float dbToLinear(float db);
  float linearToDb(float linear);
};

#endif  // EFFECT_LIMITER_HPP