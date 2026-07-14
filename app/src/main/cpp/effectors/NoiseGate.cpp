#include "NoiseGate.hpp"

#include <algorithm>

NoiseGate::NoiseGate(float threshold, float attack, float release, float range,
                     float sampleRate)
    : DynamicProcessor(threshold, attack, release, sampleRate),
      m_rangeDb(std::clamp(range, -120.0f, 0.0f)),
      m_gain(1.0f) {}

void NoiseGate::process(const float* inputBuffer, float* outputBuffer,
                        int numSamples) {
  if (m_isEnabled) {
    for (int i = 0; i < numSamples; ++i) {
      float input = std::abs(inputBuffer[i]);

      // Envelope follower (Peak detection) - same as base class
      if (input > m_envelope) {
        m_envelope = m_attackCoef * m_envelope + (1.0f - m_attackCoef) * input;
      } else {
        m_envelope =
            m_releaseCoef * m_envelope + (1.0f - m_releaseCoef) * input;
      }

      // Compute gain reduction via derived class
      float gainDb = computeGain(linearToDb(m_envelope), input, m_attackCoef,
                                 m_releaseCoef);

      // NoiseGate-specific: Smooth gain transition
      float targetGain = dbToLinear(gainDb);
      float coef = (targetGain > m_gain) ? m_attackCoef : m_releaseCoef;
      m_gain = coef * m_gain + (1.0f - coef) * targetGain;

      // Apply smoothed gain
      outputBuffer[i] = inputBuffer[i] * m_gain;
    }
  } else if (inputBuffer != outputBuffer) {
    // Bypass: copy input to output
    std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
  }
}

void NoiseGate::setRange(float range) {
  m_rangeDb = std::clamp(range, -120.0f, 0.0f);
}

float NoiseGate::computeGain(float envDb, float input, float attackCoef,
                             float releaseCoef) {
  (void)input;
  (void)attackCoef;
  (void)releaseCoef;

  // Gate behavior: fully open above threshold, attenuate to range below.
  if (envDb < m_thresholdDb) {
    return m_rangeDb;
  }
  return 0.0f;
}