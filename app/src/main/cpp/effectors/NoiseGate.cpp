#include "NoiseGate.hpp"

#include <algorithm>

NoiseGate::NoiseGate(float threshold, float attack, float release, float range,
                     float sampleRate)
    : DynamicProcessor(threshold, attack, release, sampleRate),
      m_rangeDb(range),
      m_gain(1.0f) {}

void NoiseGate::process(float* buffer, int numSamples) {
  for (int i = 0; i < numSamples; ++i) {
    float input = std::abs(buffer[i]);

    // Envelope follower (Peak detection) - same as base class
    if (input > m_envelope) {
      m_envelope = m_attackCoef * m_envelope + (1.0f - m_attackCoef) * input;
    } else {
      m_envelope = m_releaseCoef * m_envelope + (1.0f - m_releaseCoef) * input;
    }

    // Compute gain reduction via derived class
    float gainDb = computeGain(m_envelope, input, m_attackCoef, m_releaseCoef);

    // NoiseGate-specific: Smooth gain transition
    float targetGain = dbToLinear(gainDb);
    float coef = (m_envelope > m_gain) ? m_attackCoef : m_releaseCoef;
    m_gain = coef * m_gain + (1.0f - coef) * targetGain;

    // Apply smoothed gain
    buffer[i] *= m_gain;
  }
}

void NoiseGate::setRange(float range) { m_rangeDb = range; }

float NoiseGate::computeGain(float envDb, float input, float attackCoef,
                             float releaseCoef) {
  (void)input;
  (void)attackCoef;
  (void)releaseCoef;

  float gainDb = 0.0f;

  if (envDb < m_thresholdDb) {
    // Signal is below threshold - apply attenuation
    gainDb = m_rangeDb * (m_thresholdDb - envDb) / m_thresholdDb;
    gainDb = std::max(gainDb, m_rangeDb);  // Clamp to max range
  } else {
    // Signal is above threshold - gate is open (full gain)
    gainDb = 0.0f;
  }

  return gainDb;
}