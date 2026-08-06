#include "RNNoiseProcessor.hpp"

#include <algorithm>

RNNoiseProcessor::RNNoiseProcessor() {
  mFrameSize = rnnoise_get_frame_size();
  if (mFrameSize > 0) {
    m_scaledInputBuffer.resize(mFrameSize);
    // model == nullptr uses the built-in model initialized from rnnoise_data.
    mRnnoiseState = rnnoise_create(nullptr);
  }
}

RNNoiseProcessor::~RNNoiseProcessor() {
  if (mRnnoiseState != nullptr) {
    rnnoise_destroy(mRnnoiseState);
    mRnnoiseState = nullptr;
  }
}

void RNNoiseProcessor::process(const float* inputBuffer, float* outputBuffer,
                               int numSamples) {
  if (numSamples <= 0 || inputBuffer == nullptr || outputBuffer == nullptr) {
    return;
  }

  auto inputPeak = 1e-8f;  // Initialize to a small value to avoid log of zero
  for (int i = 0; i < numSamples; ++i) {
    inputPeak = std::max(inputPeak, std::abs(inputBuffer[i]));
  }
  m_inputPeakDb.store(20.0f * std::log10(inputPeak));

  if (mIsEnabled && mRnnoiseState != nullptr && mFrameSize > 0 &&
      numSamples == mFrameSize && mSampleRate == 48000.0f) {
    // RNNoise processes PCM-scale floats (int16 range, ~±32768), not normalized
    // float samples (±1.0) used by the Oboe pipeline. Scale up before
    // processing and back down afterward. A separate buffer is required because
    // inputBuffer and outputBuffer may alias (in-place processing in the
    // chain).
    constexpr float kPcmScale = 32768.0f;
    constexpr float kPcmScaleInv = 1.0f / kPcmScale;

    for (int i = 0; i < numSamples; ++i) {
      m_scaledInputBuffer[i] = inputBuffer[i] * kPcmScale;
    }
    mLastVadProbability = rnnoise_process_frame(mRnnoiseState, outputBuffer,
                                                m_scaledInputBuffer.data());
    auto outputPeak = 1e-8f;
    for (int i = 0; i < numSamples; ++i) {
      outputBuffer[i] = outputBuffer[i] * kPcmScaleInv;
      outputPeak = std::max(outputPeak, std::abs(outputBuffer[i]));
    }
    m_outputPeakDb.store(20.0f * std::log10(outputPeak));
  } else {
    if (inputBuffer != outputBuffer) {
      std::copy(inputBuffer, inputBuffer + numSamples, outputBuffer);
    }
    m_outputPeakDb.store(m_inputPeakDb.load());
  }
}
