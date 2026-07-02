#include "NoiseGate.hpp"

NoiseGate::NoiseGate(float threshold, float attack, float release, float range,
                     float sampleRate)
    : m_thresholdDb(threshold),
      m_attackMs(attack),
      m_releaseMs(release),
      m_rangeDb(range),
      m_sampleRate(sampleRate),
      m_envelope(0.0f),
      m_gain(1.0f) {}

void NoiseGate::process(float* buffer, int numSamples) {
  float attackCoef = std::exp(-1.0f / (m_sampleRate * m_attackMs / 1000.0f));
  float releaseCoef = std::exp(-1.0f / (m_sampleRate * m_releaseMs / 1000.0f));
  float thresholdLinear = dbToLinear(m_thresholdDb);
  float rangeLinear = dbToLinear(m_rangeDb);

  for (int i = 0; i < numSamples; ++i) {
    float input = std::abs(buffer[i]);

    // Envelope follower (Peak detection)
    if (input > m_envelope) {
      m_envelope = attackCoef * m_envelope + (1.0f - attackCoef) * input;
    } else {
      m_envelope = releaseCoef * m_envelope + (1.0f - releaseCoef) * input;
    }

    // Calculate gain reduction based on envelope vs threshold
    float envDb = linearToDb(m_envelope);
    float gainDb = 0.0f;

    if (envDb < m_thresholdDb) {
      // Signal is below threshold - apply attenuation
      gainDb = m_rangeDb * (m_thresholdDb - envDb) / m_thresholdDb;
      gainDb = std::max(gainDb, m_rangeDb);  // Clamp to max range
    } else {
      // Signal is above threshold - gate is open (full gain)
      gainDb = 0.0f;
    }

    // Smooth gain transition
    float targetGain = dbToLinear(gainDb);
    float coef = (m_envelope > m_gain) ? attackCoef : releaseCoef;
    m_gain = coef * m_gain + (1.0f - coef) * targetGain;

    // Apply gain
    buffer[i] *= m_gain;
  }
}

void NoiseGate::setThreshold(float threshold) { m_thresholdDb = threshold; }
void NoiseGate::setAttack(float attack) { m_attackMs = attack; }
void NoiseGate::setRelease(float release) { m_releaseMs = release; }
void NoiseGate::setRange(float range) { m_rangeDb = range; }

float NoiseGate::dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

float NoiseGate::linearToDb(float linear) {
  if (linear <= 1e-9f) return -100.0f;  // Floor for stability
  return 20.0f * std::log10(linear);
}