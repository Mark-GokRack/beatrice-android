#include "Limiter.hpp"

#include <algorithm>

Limiter::Limiter(float threshold, float attack, float release, float sampleRate)
    : DynamicProcessor(threshold, attack, release, sampleRate) {}

void Limiter::process(const float* inputBuffer, float* outputBuffer,
                      int numSamples) {
  if (m_isEnabled) {
    // Delegate all processing to base class (envelope follower + gain)
    DynamicProcessor::process(inputBuffer, outputBuffer, numSamples);

    // Enforce hard output ceiling so peaks do not exceed the limiter threshold.
    const float thresholdLinear = dbToLinear(m_thresholdDb);
    bool hardClipActive = false;
    for (int i = 0; i < numSamples; ++i) {
      const float clampedOutput =
          std::clamp(outputBuffer[i], -thresholdLinear, thresholdLinear);
      hardClipActive = hardClipActive || (clampedOutput != outputBuffer[i]);
      outputBuffer[i] = clampedOutput;
    }

    m_hardClipActive.store(hardClipActive);
  } else if (inputBuffer != outputBuffer) {
    // Bypass: copy input to output
    std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
  }

  if (!m_isEnabled) {
    publishBypassMeterState(inputBuffer, numSamples);
    m_hardClipActive.store(false);
  }
}

float Limiter::computeGain(float envDb, float input, float attackCoef,
                           float releaseCoef) {
  (void)attackCoef;
  (void)releaseCoef;

  // Use the larger of envelope and current input peak for faster limiting.
  const float detectorDb = std::max(envDb, linearToDb(input));

  float gainDb = 0.0f;
  if (detectorDb > m_thresholdDb) {
    // Infinite-ratio style limiting above threshold.
    gainDb = m_thresholdDb - detectorDb;
  }

  return gainDb;
}