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
    for (int i = 0; i < numSamples; ++i) {
      outputBuffer[i] =
          std::clamp(outputBuffer[i], -thresholdLinear, thresholdLinear);
    }
  } else if (inputBuffer != outputBuffer) {
    // Bypass: copy input to output
    std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
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