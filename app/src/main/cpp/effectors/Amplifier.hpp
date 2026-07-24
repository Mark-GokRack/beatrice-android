#ifndef EFFECT_AMPLIFIER_HPP
#define EFFECT_AMPLIFIER_HPP

#include "AudioEffector.hpp"

/**
 * @brief A simple audio amplifier class.
 *
 * This class applies a gain factor to the input audio signal, effectively
 * amplifying or attenuating it. The gain is specified in decibels (dB).
 */
class Amplifier : public AudioEffector {
 public:
  Amplifier(float gainDb = 0.0f);

  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;

  void setGain(float gainDb);
  float getGain() const { return m_gainDb; }

  void setSampleRate(float sampleRate) override {
    (void)sampleRate;  // Sample rate is not used in this effector
  }

  void setEnabled(bool enabled) override { m_isEnabled = enabled; }

  bool isEnabled() const override { return m_isEnabled; }

 private:
  float m_gainLinear = 1.0f;  // Linear gain factor
  float m_gainDb;             // Gain in decibels
  bool m_isEnabled = false;   // Whether the amplifier is enabled
  float dbToLinear(float db);
};

#endif  // EFFECT_AMPLIFIER_HPP