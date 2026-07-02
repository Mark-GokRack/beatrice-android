#include "Compressor.hpp"

Compressor::Compressor(float threshold, float ratio, float attack,
                       float release, float makeupGain, float sampleRate)
    : DynamicProcessor(threshold, attack, release, sampleRate),
      m_ratio(ratio),
      m_makeupGainDb(makeupGain),
      m_makeupGainLinear(dbToLinear(makeupGain)) {}

void Compressor::process(float* buffer, int numSamples) {
  // Call base class process, which handles envelope follower and gain
  // calculation
  DynamicProcessor::process(buffer, numSamples);

  // Apply makeup gain to the already-compressed buffer
  for (int i = 0; i < numSamples; ++i) {
    buffer[i] *= m_makeupGainLinear;
  }
}

void Compressor::setRatio(float ratio) { m_ratio = ratio; }
void Compressor::setMakeupGain(float makeupGain) {
  m_makeupGainDb = makeupGain;
  m_makeupGainLinear = dbToLinear(makeupGain);
}

float Compressor::computeGain(float envDb, float input, float attackCoef,
                              float releaseCoef) {
  (void)input;  // Not used directly in gain calculation
  (void)attackCoef;
  (void)releaseCoef;

  float gainDb = 0.0f;

  if (envDb > m_thresholdDb) {
    gainDb = (m_thresholdDb - envDb) * (1.0f - 1.0f / m_ratio);
  }

  return gainDb;
}
