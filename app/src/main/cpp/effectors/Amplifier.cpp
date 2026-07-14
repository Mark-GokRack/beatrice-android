#include "Amplifier.hpp"

#include <algorithm>
#include <cmath>

Amplifier::Amplifier(float gainDb)
    : m_gainDb(gainDb), m_gainLinear(dbToLinear(gainDb)) {}

void Amplifier::process(const float* inputBuffer, float* outputBuffer,
                        int numSamples) {
  if (m_isEnabled) {
    float gainLinear = dbToLinear(m_gainDb);
    for (int i = 0; i < numSamples; ++i) {
      outputBuffer[i] = inputBuffer[i] * gainLinear;
    }
  } else if (inputBuffer != outputBuffer) {
    std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
  }
}

void Amplifier::setGain(float gainDb) {
  m_gainDb = gainDb;
  m_gainLinear = dbToLinear(gainDb);
}

float Amplifier::dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }