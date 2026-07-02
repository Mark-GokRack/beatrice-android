#ifndef EFFECT_PARAMETRIC_EQUALIZER_HPP
#define EFFECT_PARAMETRIC_EQUALIZER_HPP

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <vector>

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
class ParametricEqualizer {
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
   * @param buffer Pointer to the audio buffer (processed in-place).
   * @param numSamples Number of samples in the buffer.
   */
  void process(float* buffer, int numSamples);

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
   * @brief Sets the sample rate.
   *
   * @param sampleRate Sampling rate in Hz.
   */
  void setSampleRate(float sampleRate);

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
};

#endif  // EFFECT_PARAMETRIC_EQUALIZER_HPP
