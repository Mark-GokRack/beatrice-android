#ifndef EFFECT_RNNOISE_PROCESSOR_HPP
#define EFFECT_RNNOISE_PROCESSOR_HPP

#include <atomic>
#include <vector>

#include "AudioEffector.hpp"
#include "rnnoise.h"

class RNNoiseProcessor : public AudioEffector {
 public:
  RNNoiseProcessor();
  ~RNNoiseProcessor() override;

  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;

  void setSampleRate(float sampleRate) override { mSampleRate = sampleRate; }
  float getSampleRate() const { return mSampleRate; }

  void setEnabled(bool enabled) override { mIsEnabled = enabled; }
  bool isEnabled() const override { return mIsEnabled; }

  int getFrameSize() const { return mFrameSize; }
  float getLastVadProbability() const { return mLastVadProbability; }
  bool isReady() const { return mRnnoiseState != nullptr; }

  float getInputPeakDb() const { return m_inputPeakDb.load(); }
  float getOutputPeakDb() const { return m_outputPeakDb.load(); }

 private:
  DenoiseState* mRnnoiseState = nullptr;
  int mFrameSize = 0;
  float mSampleRate = 48000.0f;
  bool mIsEnabled = false;
  float mLastVadProbability = 0.0f;
  std::atomic<float> m_outputPeakDb = -100.0f;
  std::atomic<float> m_inputPeakDb = -100.0f;
  std::vector<float> m_scaledInputBuffer;
};

#endif  // EFFECT_RNNOISE_PROCESSOR_HPP