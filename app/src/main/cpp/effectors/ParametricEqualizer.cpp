#include "ParametricEqualizer.hpp"

#include <algorithm>
#include <cmath>

ParametricEqualizer::ParametricEqualizer(float sampleRate, int numBands)
    : m_sampleRate(sampleRate),
      m_numBands(std::min(std::max(numBands, 1), 8)),
      m_bands(m_numBands, {1000.0f, 1.0f, 0.0f}),
      m_z1(m_numBands, std::vector<float>(1, 0.0f)),
      m_z2(m_numBands, std::vector<float>(1, 0.0f)),
      m_coeffs(m_numBands, BiquadCoeffs{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}),
      m_cachedSampleRate(sampleRate),
      m_cacheValid(false) {}

void ParametricEqualizer::setNumBands(int numBands) {
  int bands = std::min(std::max(numBands, 1), 8);
  if (bands != m_numBands) {
    m_numBands = bands;
    m_bands.assign(m_numBands, {1000.0f, 1.0f, 0.0f});
    m_z1.assign(m_numBands, std::vector<float>(1, 0.0f));
    m_z2.assign(m_numBands, std::vector<float>(1, 0.0f));
    m_coeffs.assign(m_numBands,
                    BiquadCoeffs{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f});
    computeAllCoeffs();
    m_cacheValid = false;
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
  m_cacheValid = false;
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
  m_cacheValid = false;
}

void ParametricEqualizer::process(const float* inputBuffer, float* outputBuffer,
                                  int numSamples) {
  if (m_isEnabled) {
    // Process through all bands in cascade
    const float* currentInput = inputBuffer;
    float* currentOutput = outputBuffer;
    for (int bandIndex = 0; bandIndex < m_numBands; ++bandIndex) {
      for (int i = 0; i < numSamples; ++i) {
        float output = processBiquad(currentInput[i], bandIndex);
        currentOutput[i] = output;
      }
      currentInput =
          currentOutput;  // Output of this band becomes input for the next
    }
  } else if (inputBuffer != outputBuffer) {
    // Bypass: copy input to output
    std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
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
  m_cacheValid = true;
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

float ParametricEqualizer::computeBiquadMagnitude(int bandIndex,
                                                  float frequency) const {
  if (bandIndex < 0 || bandIndex >= m_numBands) return 1.0f;
  if (frequency <= 0.0f || frequency >= m_sampleRate * 0.5f) return 1.0f;

  const auto& coeffs = m_coeffs[bandIndex];

  // ω = 2πf/fs
  float omega = 2.0f * static_cast<float>(M_PI) * frequency / m_sampleRate;

  // cos(ω) and cos(2ω)
  float cosW = std::cos(omega);
  float cos2W = std::cos(2.0f * omega);
  float sinW = std::sin(omega);
  float sin2W = std::sin(2.0f * omega);

  // Numerator: b0 + b1·e^(-jω) + b2·e^(-j2ω)
  float realNum = coeffs.b0 + coeffs.b1 * cosW + coeffs.b2 * cos2W;
  float imagNum = -(coeffs.b1 * sinW + coeffs.b2 * sin2W);

  // Denominator: a0 + a1·e^(-jω) + a2·e^(-j2ω)  (a0 == 1.0)
  float realDen = 1.0f + coeffs.a1 * cosW + coeffs.a2 * cos2W;
  float imagDen = -(coeffs.a1 * sinW + coeffs.a2 * sin2W);

  // |H(ω)|
  float mag = std::sqrt(realNum * realNum + imagNum * imagNum) /
              std::sqrt(realDen * realDen + imagDen * imagDen);

  return mag;
}

bool ParametricEqualizer::isCacheValid(
    const std::vector<float>& frequencies) const {
  if (!m_cacheValid) return false;
  if (m_cachedFrequencies.size() != frequencies.size()) return false;
  if (m_cachedResponse.size() != frequencies.size()) return false;
  if (m_cachedBands.size() != static_cast<size_t>(m_numBands)) return false;

  // Check if frequencies match
  for (size_t i = 0; i < frequencies.size(); ++i) {
    if (frequencies[i] != m_cachedFrequencies[i]) return false;
  }

  // Check if sample rate changed
  if (m_sampleRate != m_cachedSampleRate) return false;

  // Check if any band parameters changed
  for (int i = 0; i < m_numBands; ++i) {
    if (m_bands[i].centerFrequency != m_cachedBands[i].centerFrequency ||
        m_bands[i].Q != m_cachedBands[i].Q ||
        m_bands[i].gainDb != m_cachedBands[i].gainDb) {
      return false;
    }
  }

  return true;
}

std::vector<float> ParametricEqualizer::computeFrequencyResponse(
    const std::vector<float>& frequencies) {
  // Check cache
  if (isCacheValid(frequencies)) {
    return m_cachedResponse;
  }

  // Compute response
  std::vector<float> response(frequencies.size(), 0.0f);

  for (size_t i = 0; i < frequencies.size(); ++i) {
    float magLinear = 1.0f;

    // Cascade all bands: multiply magnitudes
    for (int band = 0; band < m_numBands; ++band) {
      magLinear *= computeBiquadMagnitude(band, frequencies[i]);
    }

    // Convert to dB
    if (magLinear > 1e-10f) {
      response[i] = 20.0f * std::log10(magLinear);
    } else {
      response[i] = -120.0f;  // Near-zero magnitude floor
    }
  }

  // Update cache
  m_cachedFrequencies = frequencies;
  m_cachedBands = m_bands;
  m_cachedSampleRate = m_sampleRate;
  m_cachedResponse = response;
  m_cacheValid = true;

  return response;
}

float ParametricEqualizer::clamp(float value, float minVal, float maxVal) {
  return std::max(minVal, std::min(value, maxVal));
}
