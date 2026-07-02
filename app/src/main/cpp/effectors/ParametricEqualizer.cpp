#include "ParametricEqualizer.hpp"

#include <algorithm>
#include <cmath>

ParametricEqualizer::ParametricEqualizer(float sampleRate, int numBands)
    : m_sampleRate(sampleRate),
      m_numBands(std::min(std::max(numBands, 1), 8)),
      m_bands(m_numBands, {1000.0f, 1.0f, 0.0f}),
      m_z1(m_numBands, std::vector<float>(0.0f)),
      m_z2(m_numBands, std::vector<float>(0.0f)),
      m_coeffs(m_numBands) {}

void ParametricEqualizer::setNumBands(int numBands) {
  int bands = std::min(std::max(numBands, 1), 8);
  if (bands != m_numBands) {
    m_numBands = bands;
    m_bands.assign(m_numBands, {1000.0f, 1.0f, 0.0f});
    m_z1.assign(m_numBands, std::vector<float>(0.0f));
    m_z2.assign(m_numBands, std::vector<float>(0.0f));
    m_coeffs.assign(m_numBands);
    computeAllCoeffs();
  }
}

void ParametricEqualizer::setBand(int bandIndex, float centerFrequency, float Q,
                                  float gainDb) {
  if (bandIndex < 0 || bandIndex >= m_numBands) return;

  m_bands[bandIndex].centerFrequency = clamp(centerFrequency, 20.0f, 20000.0f);
  m_bands[bandIndex].Q = clamp(Q, 0.1f, 10.0f);
  m_bands[bandIndex].gainDb = clamp(gainDb, -24.0f, 24.0f);

  computePeakingCoeffs(bandIndex, m_bands[bandIndex].centerFrequency,
                       m_bands[bandIndex].Q, m_bands[bandIndex].gainDb);
}

float ParametricEqualizer::getCenterFrequency(int bandIndex) const {
  if (bandIndex < 0 || bandIndex >= m_numBands) return 1000.0f;
  return m_bands[bandIndex].centerFrequency;
}

float ParametricEqualizer::getQ(int bandIndex) const {
  if (bandIndex < 0 || bandIndex >= m_numBands) return 1.0f;
  return m_bands[bandIndex].Q;
}

float ParametricEqualizer::getGain(int bandIndex) const {
  if (bandIndex < 0 || bandIndex >= m_numBands) return 0.0f;
  return m_bands[bandIndex].gainDb;
}

void ParametricEqualizer::setSampleRate(float sampleRate) {
  m_sampleRate = sampleRate;
  computeAllCoeffs();
}

void ParametricEqualizer::process(float* buffer, int numSamples) {
  // Process through all bands in cascade
  for (int bandIndex = 0; bandIndex < m_numBands; ++bandIndex) {
    float* currentBuffer = buffer;

    for (int i = 0; i < numSamples; ++i) {
      float output = processBiquad(currentBuffer[i], bandIndex);
      buffer[i] = output;
    }
  }
}

void ParametricEqualizer::computePeakingCoeffs(int bandIndex,
                                               float centerFrequency, float Q,
                                               float gainDb) {
  if (bandIndex < 0 || bandIndex >= m_numBands) return;
  if (centerFrequency <= 0.0f || Q <= 0.0f || m_sampleRate <= 0.0f) return;

  float omega = 2.0f * M_PI * centerFrequency / m_sampleRate;
  float sinOmega = std::sin(omega);
  float cosOmega = std::cos(omega);
  float alpha = sinOmega / (2.0f * Q);

  float gainLinear = std::pow(10.0f, gainDb / 20.0f);
  float A = gainLinear;

  float b0, b1, b2, a0, a1, a2;

  // Peaking EQ biquad coefficients (Robert Bristow-Johnson formula)
  b0 = 1.0f + alpha * A;
  b1 = -2.0f * cosOmega;
  b2 = 1.0f - alpha * A;
  a0 = 1.0f + alpha / A;
  a1 = -2.0f * cosOmega;
  a2 = 1.0f - alpha / A;

  // Normalize by a0
  m_coeffs[bandIndex].b0 = b0 / a0;
  m_coeffs[bandIndex].b1 = b1 / a0;
  m_coeffs[bandIndex].b2 = b2 / a0;
  m_coeffs[bandIndex].a0 = 1.0f;  // Already normalized
  m_coeffs[bandIndex].a1 = a1 / a0;
  m_coeffs[bandIndex].a2 = a2 / a0;
}

void ParametricEqualizer::computeAllCoeffs() {
  for (int i = 0; i < m_numBands; ++i) {
    computePeakingCoeffs(i, m_bands[i].centerFrequency, m_bands[i].Q,
                         m_bands[i].gainDb);
  }
}

float ParametricEqualizer::processBiquad(float input, int bandIndex) {
  // Direct form II transposed biquad filter
  float output = m_coeffs[bandIndex].b0 * input + m_z1[bandIndex][0];
  m_z1[bandIndex][0] = m_coeffs[bandIndex].b1 * input -
                       m_coeffs[bandIndex].a1 * output + m_z2[bandIndex][0];
  m_z2[bandIndex][0] =
      m_coeffs[bandIndex].b2 * input - m_coeffs[bandIndex].a2 * output;

  return output;
}

float ParametricEqualizer::clamp(float value, float minVal, float maxVal) {
  return std::max(minVal, std::min(value, maxVal));
}
