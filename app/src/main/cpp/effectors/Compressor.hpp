#ifndef EFFECT_COMPRESSOR_HPP
#define EFFECT_COMPRESSOR_HPP

#include "DynamicProcessor.hpp"

/**
 * @brief A simple audio compressor class.
 *
 * Extends DynamicProcessor to implement compression with a configurable ratio
 * and makeup gain.
 */
class Compressor : public DynamicProcessor {
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
   * Applies compression followed by makeup gain.
   *
   * @param inputBuffer Pointer to the input audio buffer.
   * @param outputBuffer Pointer to the output audio buffer.
   * @param numSamples Number of samples in the buffer.
   */
  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;

  // Setters for parameters
  void setRatio(float ratio);
  void setMakeupGain(float makeupGain);
  float getRatio() const { return m_ratio; }
  float getMakeupGain() const { return m_makeupGainDb; }

 protected:
  // DynamicProcessor base handles threshold, attack, release, sampleRate,
  // envelope

 private:
  // Parameters
  float m_ratio;
  float m_makeupGainDb;
  float m_makeupGainLinear;

  // Override from DynamicProcessor
  float computeGain(float envDb, float input, float attackCoef,
                    float releaseCoef) override;
};

#endif  // EFFECT_COMPRESSOR_HPP
