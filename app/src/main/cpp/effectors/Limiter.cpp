#include "Limiter.hpp"

Limiter::Limiter(float threshold, float attack, float release, float sampleRate)
    : DynamicProcessor(threshold, attack, release, sampleRate) {}

void Limiter::process(float* buffer, int numSamples) {
  // Delegate all processing to base class (envelope follower + gain)
  DynamicProcessor::process(buffer, numSamples);
}

float Limiter::computeGain(float envDb, float input, float attackCoef,
                           float releaseCoef) {
  (void)input;
  (void)attackCoef;
  (void)releaseCoef;

  float gainDb = 0.0f;

  if (envDb > m_thresholdDb) {
    // Very high ratio for limiter (effectively infinity:1)
    gainDb = (m_thresholdDb - envDb) * (1.0f - 1.0f / kLimiterRatio);
  }

  return gainDb;
}