#include "DynamicProcessor.hpp"

#include <algorithm>
#include <cmath>

DynamicProcessor::DynamicProcessor(float threshold, float attack, float release,
                                   float sampleRate)
    : m_thresholdDb(threshold),
      m_attackMs(attack),
      m_releaseMs(release),
      m_sampleRate(sampleRate),
      m_attackCoef(std::exp(-1.0f / (sampleRate * attack / 1000.0f))),
      m_releaseCoef(std::exp(-1.0f / (sampleRate * release / 1000.0f))),
      m_envelope(0.0f),
      m_outputPeakDb(-100.0f),
      m_detectorLevelDb(-100.0f),
      m_gainReductionDb(0.0f),
      m_inputPeakDb(-100.0f),
      m_isActive(false) {}

void DynamicProcessor::process(const float* inputBuffer, float* outputBuffer,
                               int numSamples) {
  float inputPeak = 0.0f;
  float outputPeak = 0.0f;
  float gainReductionDb = 0.0f;

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

    inputPeak = std::max(inputPeak, input);
    outputPeak = std::max(outputPeak, std::abs(outputBuffer[i]));
    gainReductionDb = std::min(gainReductionDb, gainDb);
  }

  publishMeterState(m_envelope, inputPeak, outputPeak, gainReductionDb,
                    gainReductionDb < -0.01f);
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

void DynamicProcessor::publishMeterState(float detectorLevelLinear,
                                         float inputPeakLinear,
                                         float outputPeakLinear,
                                         float gainReductionDb, bool isActive) {
  m_detectorLevelDb.store(linearToDb(detectorLevelLinear));
  m_gainReductionDb.store(gainReductionDb);
  m_inputPeakDb.store(linearToDb(inputPeakLinear));
  m_outputPeakDb.store(linearToDb(outputPeakLinear));
  m_isActive.store(isActive);
}

void DynamicProcessor::publishBypassMeterState(const float* inputBuffer,
                                               int numSamples) {
  float inputPeak = 0.0f;
  for (int i = 0; i < numSamples; ++i) {
    inputPeak = std::max(inputPeak, std::abs(inputBuffer[i]));
  }

  publishMeterState(inputPeak, inputPeak, inputPeak, 0.0f, false);
}
