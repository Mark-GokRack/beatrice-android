#ifndef EFFECT_NOISE_GATE_HPP
#define EFFECT_NOISE_GATE_HPP

#include <algorithm>
#include <cmath>

/**
 * @brief A noise gate class for audio signal processing.
 *
 * A noise gate attenuates signals below a set threshold level and passes
 * signals above the threshold. It is used to reduce background noise by
 * "gating" unwanted low-level audio.
 */
class NoiseGate {
 public:
  /**
   * @brief Constructor for NoiseGate.
   *
   * @param threshold Threshold in dB (level below which noise is attenuated).
   * @param attack Attack time in milliseconds (time to open gate).
   * @param release Release time in milliseconds (time to close gate).
   * @param range Maximum attenuation in dB when gate is closed.
   * @param sampleRate Sampling rate in Hz.
   */
  NoiseGate(float threshold = -40.0f, float attack = 5.0f,
            float release = 50.0f, float range = -80.0f,
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
  void setRange(float range);

 private:
  // Parameters
  float m_thresholdDb;
  float m_attackMs;
  float m_releaseMs;
  float m_rangeDb;

  // Internal state
  float m_envelope;
  float m_gain;
  float m_sampleRate;

  // Helper functions
  float dbToLinear(float db);
  float linearToDb(float linear);
};

#endif  // EFFECT_NOISE_GATE_HPP