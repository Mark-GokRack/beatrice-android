#ifndef EFFECT_PARAMETRIC_EQUALIZER_HPP
#define EFFECT_PARAMETRIC_EQUALIZER_HPP

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <vector>

#include "AudioEffector.hpp"

/**
 * @brief Biquad filter coefficient structure.
 */
struct BiquadCoeffs {
  float b0, b1, b2;
  float a0, a1, a2;
};

/**
 * @brief A parametric equalizer using IIR biquad filters.
 *
 * Supports multiple bands, each with adjustable center frequency, Q factor, and
 * gain. Uses cascaded biquad IIR filters for efficient processing.
 */
class ParametricEqualizer : public AudioEffector {
 public:
  /**
   * @brief Represents a single EQ band.
   */
  struct Band {
    float centerFrequency;  // Center frequency in Hz (20 - 20000)
    float Q;                // Quality factor (0.1 - 10.0)
    float gainDb;           // Gain in dB (-24.0 to +24.0)
  };

  /**
   * @brief Constructor for ParametricEqualizer.
   *
   * @param sampleRate Sampling rate in Hz.
   * @param numBands Number of EQ bands (default 3).
   */
  ParametricEqualizer(float sampleRate = 44100.0f, int numBands = 3);

  /**
   * @brief Processes a buffer of audio samples.
   *
   * @param inputBuffer Pointer to the input audio buffer.
   * @param outputBuffer Pointer to the output audio buffer.
   * @param numSamples Number of samples in the buffer.
   */
  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;

  /**
   * @brief Sets the sample rate.
   *
   * @param sampleRate Sampling rate in Hz.
   */
  void setSampleRate(float sampleRate) override;

  /**
   * @brief Sets the number of EQ bands.
   *
   * @param numBands Number of bands (1-8).
   */
  void setNumBands(int numBands);

  /**
   * @brief Sets the parameters for a specific band.
   *
   * @param bandIndex Index of the band (0-based).
   * @param centerFrequency Center frequency in Hz.
   * @param Q Quality factor.
   * @param gainDb Gain in dB.
   */
  void setBand(int bandIndex, float centerFrequency, float Q, float gainDb);

  /**
   * @brief Gets the center frequency of a band.
   */
  float getCenterFrequency(int bandIndex) const;

  /**
   * @brief Gets the Q factor of a band.
   */
  float getQ(int bandIndex) const;

  /**
   * @brief Gets the gain of a band in dB.
   */
  float getGain(int bandIndex) const;

  /**
   * @brief Computes the frequency response at specified points.
   *
   * Uses the biquad coefficients to calculate the magnitude response
   * via the complex transfer function H(e^(jω)).
   *
   * @param frequencies Vector of frequency points in Hz (log-spaced
   * recommended).
   * @return Vector of magnitude responses in dB.
   */
  std::vector<float> computeFrequencyResponse(
      const std::vector<float>& frequencies);

  /**
   * @brief Enables or disables the equalizer.
   *
   * @param enabled True to enable, false to bypass.
   */

  void setEnabled(bool enabled) override { m_isEnabled = enabled; }

  /**
   * @brief Checks if the equalizer is enabled.
   *
   * @return True if enabled, false if bypassed.
   */
  bool isEnabled() const override { return m_isEnabled; }

 private:
  // Parameters
  float m_sampleRate;
  int m_numBands;
  std::vector<Band> m_bands;

  // Biquad filter states
  std::vector<std::vector<float>> m_z1;  // Delay line 1 per band
  std::vector<std::vector<float>> m_z2;  // Delay line 2 per band

  // Filter coefficients per band
  std::vector<BiquadCoeffs> m_coeffs;

  // Helper functions
  void computePeakingCoeffs(int bandIndex, float centerFrequency, float Q,
                            float gainDb);
  void computeAllCoeffs();
  float processBiquad(float input, int bandIndex);
  float clamp(float value, float minVal, float maxVal);
  float computeBiquadMagnitude(int bandIndex, float frequency) const;
  bool isCacheValid(const std::vector<float>& frequencies) const;

  // Cache members
  mutable std::vector<float> m_cachedResponse;
  mutable std::vector<float> m_cachedFrequencies;
  mutable std::vector<Band> m_cachedBands;
  float m_cachedSampleRate;
  bool m_cacheValid;
  bool m_isEnabled = true;  // Default to enabled
};

#endif  // EFFECT_PARAMETRIC_EQUALIZER_HPP
