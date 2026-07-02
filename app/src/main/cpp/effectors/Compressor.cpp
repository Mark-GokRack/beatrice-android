#include "Compressor.hpp"

Compressor::Compressor(float threshold, float ratio, float attack,
                       float release, float makeupGain, float sampleRate)
    : m_thresholdDb(threshold),
      m_ratio(ratio),
      m_attackMs(attack),
      m_releaseMs(release),
      m_makeupGainDb(makeupGain),
      m_sampleRate(sampleRate),
      m_envelope(0.0f) {}

void Compressor::process(float* buffer, int numSamples) {
  float attackCoef = std::exp(-1.0f / (m_sampleRate * m_attackMs / 1000.0f));
  float releaseCoef = std::exp(-1.0f / (m_sampleRate * m_releaseMs / 1000.0f));
  float makeupGainLinear = dbToLinear(m_makeupGainDb);

  for (int i = 0; i < numSamples; ++i) {
    float input = std::abs(buffer[i]);

    // Envelope follower (Peak detection)
    if (input > m_envelope) {
      m_envelope = attackCoef * m_envelope + (1.0f - attackCoef) * input;
    } else {
      m_envelope = releaseCoef * m_envelope + (1.0f - releaseCoef) * input;
    }

    float envDb = linearToDb(m_envelope);
    float gainDb = 0.0f;

    if (envDb > m_thresholdDb) {
      gainDb = (m_thresholdDb - envDb) * (1.0f - 1.0f / m_ratio);
    }

    float gainLinear = dbToLinear(gainDb);
    buffer[i] *= gainLinear * makeupGainLinear;
  }
}

void Compressor::setThreshold(float threshold) { m_thresholdDb = threshold; }
void Compressor::setRatio(float ratio) { m_ratio = ratio; }
void Compressor::setAttack(float attack) { m_attackMs = attack; }
void Compressor::setRelease(float release) { m_releaseMs = release; }
void Compressor::setMakeupGain(float makeupGain) {
  m_makeupGainDb = makeupGain;
}

float Compressor::dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

float Compressor::linearToDb(float linear) {
  if (linear <= 1e-9f) return -100.0f;  // Floor for stability
  return 20.0f * std::log10(linear);
}
