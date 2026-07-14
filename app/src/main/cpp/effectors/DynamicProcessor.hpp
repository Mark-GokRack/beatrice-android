#ifndef EFFECT_DYNAMIC_PROCESSOR_HPP
#define EFFECT_DYNAMIC_PROCESSOR_HPP

#include "AudioEffector.hpp"

/**
 * @brief Abstract base class for dynamic range processors.
 *
 * Provides common functionality shared by Compressor, Limiter, and NoiseGate:
 * - Envelope follower (peak detection with attack/release smoothing)
 * - dB <-> linear conversion helpers
 *
 * Derived classes implement computeGain() to define their specific gain
 * reduction behavior.
 */
class DynamicProcessor : public AudioEffector {
 public:
  /**
   * @brief Constructor.
   *
   * @param threshold Threshold in dB.
   * @param attack Attack time in milliseconds.
   * @param release Release time in milliseconds.
   * @param sampleRate Sampling rate in Hz.
   */
  DynamicProcessor(float threshold, float attack, float release,
                   float sampleRate);

  /**
   * @brief Virtual destructor.
   */
  ~DynamicProcessor() override = default;

  /**
   * @brief Processes a buffer of audio samples.
   *
   * Derived classes must implement computeGain() to define their specific
   * gain reduction behavior.
   *
   * @param inputBuffer Pointer to the input audio buffer.
   * @param outputBuffer Pointer to the output audio buffer.
   * @param numSamples Number of samples in the buffer.
   */
  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;

  void setSampleRate(float sampleRate) override;

  void setEnabled(bool enabled) override { m_isEnabled = enabled; }
  bool isEnabled() const override { return m_isEnabled; }

  // Setters for common parameters
  void setThreshold(float threshold);
  void setAttack(float attack);
  void setRelease(float release);

 protected:
  // Common parameters
  float m_thresholdDb;
  float m_attackMs;
  float m_releaseMs;
  float m_sampleRate;
  float m_attackCoef;
  float m_releaseCoef;

  // Internal state
  float m_envelope;

  bool m_isEnabled = false;

  /**
   * @brief Compute the gain reduction for the current envelope level.
   *
   * @param envDb Current envelope level in dB.
   * @param input Current input sample absolute value.
   * @param attackCoef Attack coefficient for smoothing.
   * @param releaseCoef Release coefficient for smoothing.
   * @return Gain reduction in dB (typically <= 0.0f).
   */
  virtual float computeGain(float envDb, float input, float attackCoef,
                            float releaseCoef) = 0;

  // Helper functions
  float dbToLinear(float db);
  float linearToDb(float linear);
};

#endif  // EFFECT_DYNAMIC_PROCESSOR_HPP
