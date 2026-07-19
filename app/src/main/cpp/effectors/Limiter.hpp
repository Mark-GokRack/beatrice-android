#ifndef EFFECT_LIMITER_HPP
#define EFFECT_LIMITER_HPP

#include <atomic>

#include "DynamicProcessor.hpp"

/**
 * @brief A simple audio limiter class.
 *
 * A limiter is a compressor with a very high ratio (effectively infinity:1)
 * that prevents the signal from exceeding a set threshold level.
 *
 * Extends DynamicProcessor with a fixed high ratio.
 */
class Limiter : public DynamicProcessor {
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
   * @param inputBuffer Pointer to the input audio buffer.
   * @param outputBuffer Pointer to the output audio buffer.
   * @param numSamples Number of samples in the buffer.
   */
  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;

  bool isHardClipActive() const { return m_hardClipActive.load(); }

 private:
  std::atomic<bool> m_hardClipActive{false};

  // Override from DynamicProcessor
  float computeGain(float envDb, float input, float attackCoef,
                    float releaseCoef) override;
};

#endif  // EFFECT_LIMITER_HPP