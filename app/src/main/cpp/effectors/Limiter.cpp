#include "Limiter.hpp"

Limiter::Limiter(float threshold, float attack, float release, float sampleRate)
    : m_thresholdDb(threshold),
      m_attackMs(attack),
      m_releaseMs(release),
      m_sampleRate(sampleRate),
      m_envelope(0.0f) {}

void Limiter::process(float* buffer, int numSamples) {
  // Very high ratio for limiter (effectively infinity:1)
  constexpr float ratio = 20.0f;

  float attackCoef = std::exp(-1.0f / (m_sampleRate * m_attackMs / 1000.0f));
  float releaseCoef = std::exp(-1.0f / (m_sampleRate * m_releaseMs / 1000.0f));
  float thresholdLinear = dbToLinear(m_thresholdDb);

  for (int i = 0; i < numSamples; ++i) {
    float input = std::abs(buffer[i]);

    // Envelope follower (Peak detection)
    if (input > m_envelope) {
      m_envelope = attackCoef * m_envelope + (1.0f - attackCoef) * input;
    } else {
      m_envelope = releaseCoef * m_envelope + (1.0f - releaseCoef) * input;
    }

    // Calculate gain reduction
    float envDb = linearToDb(m_envelope);
    float gainDb = 0.0f;

    if (envDb > m_thresholdDb) {
      gainDb = (m_thresholdDb - envDb) * (1.0f - 1.0f / ratio);
    }

    float gainLinear = dbToLinear(gainDb);
    buffer[i] *= gainLinear;
  }
}

void Limiter::setThreshold(float threshold) { m_thresholdDb = threshold; }
void Limiter::setAttack(float attack) { m_attackMs = attack; }
void Limiter::setRelease(float release) { m_releaseMs = release; }

float Limiter::dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

float Limiter::linearToDb(float linear) {
  if (linear <= 1e-9f) return -100.0f;  // Floor for stability
  return 20.0f * std::log10(linear);
}