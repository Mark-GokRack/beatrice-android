#include "DynamicProcessor.hpp"

DynamicProcessor::DynamicProcessor(float threshold, float attack, float release,
                                   float sampleRate)
    : m_thresholdDb(threshold),
      m_attackMs(attack),
      m_releaseMs(release),
      m_sampleRate(sampleRate),
      m_envelope(0.0f),
      m_attackCoef(std::exp(-1.0f / (sampleRate * attack / 1000.0f))),
      m_releaseCoef(std::exp(-1.0f / (sampleRate * release / 1000.0f))) {}

void DynamicProcessor::process(const float* inputBuffer, float* outputBuffer,
                               int numSamples) {
  for (int i = 0; i < numSamples; ++i) {
    float input = std::abs(inputBuffer[i]);

    // Envelope follower (Peak detection)
    if (input > m_envelope) {
      m_envelope = m_attackCoef * m_envelope + (1.0f - m_attackCoef) * input;
    } else {
      m_envelope = m_releaseCoef * m_envelope + (1.0f - m_releaseCoef) * input;
    }

    // Compute gain reduction via derived class
    float gainDb =
        computeGain(linearToDb(m_envelope), input, m_attackCoef, m_releaseCoef);

    // Apply gain
    float gainLinear = dbToLinear(gainDb);
    outputBuffer[i] = inputBuffer[i] * gainLinear;
  }
}

void DynamicProcessor::setSampleRate(float sampleRate) {
  m_sampleRate = sampleRate;
  m_attackCoef = std::exp(-1.0f / (m_sampleRate * m_attackMs / 1000.0f));
  m_releaseCoef = std::exp(-1.0f / (m_sampleRate * m_releaseMs / 1000.0f));
}

void DynamicProcessor::setThreshold(float threshold) {
  m_thresholdDb = threshold;
}
void DynamicProcessor::setAttack(float attack) {
  m_attackMs = attack;
  m_attackCoef = std::exp(-1.0f / (m_sampleRate * m_attackMs / 1000.0f));
}
void DynamicProcessor::setRelease(float release) {
  m_releaseMs = release;
  m_releaseCoef = std::exp(-1.0f / (m_sampleRate * m_releaseMs / 1000.0f));
}

float DynamicProcessor::dbToLinear(float db) {
  return std::pow(10.0f, db / 20.0f);
}

float DynamicProcessor::linearToDb(float linear) {
  if (linear <= 1e-9f) return -100.0f;  // Floor for stability
  return 20.0f * std::log10(linear);
}
