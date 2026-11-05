#ifndef AUDIO_CHANNEL_ROUTING_H
#define AUDIO_CHANNEL_ROUTING_H

#include <algorithm>
#include <cstddef>

enum class SecondOutputChannel {
  Processed = 0,
  Inverted = 1,
  Input = 2,
};

namespace audio_channel_routing {

template <int FixedOutputChannels, SecondOutputChannel Mode>
inline void routeSegment(const float* input, float* output, int inputChannels,
                         int outputChannels, float* inputBuffer,
                         const float* outputBuffer, size_t frameCount) {
  const int outputStride =
      FixedOutputChannels == 0 ? outputChannels : FixedOutputChannels;
  for (size_t frame = 0; frame < frameCount; ++frame) {
    const float inputSample = input[frame * inputChannels];
    const float processedSample = outputBuffer[frame];
    inputBuffer[frame] = inputSample;
    float* outputFrame = output + frame * outputStride;
    outputFrame[0] = processedSample;
    if constexpr (FixedOutputChannels != 1) {
      if constexpr (Mode == SecondOutputChannel::Processed) {
        outputFrame[1] = processedSample;
      } else if constexpr (Mode == SecondOutputChannel::Inverted) {
        outputFrame[1] = -processedSample;
      } else if constexpr (Mode == SecondOutputChannel::Input) {
        outputFrame[1] = inputSample;
      }
    }
    if constexpr (FixedOutputChannels == 0) {
      for (int channel = 2; channel < outputChannels; ++channel) {
        outputFrame[channel] = 0.0f;
      }
    }
  }
}

template <int FixedOutputChannels, SecondOutputChannel Mode>
inline void routeWrapped(const float* input, float* output, int inputChannels,
                         int outputChannels, float* inputBuffer,
                         const float* outputBuffer, size_t bufferSize,
                         size_t bufferIndex, size_t frameCount) {
  while (frameCount > 0) {
    const size_t length = std::min(frameCount, bufferSize - bufferIndex);
    routeSegment<FixedOutputChannels, Mode>(
        input, output, inputChannels, outputChannels, inputBuffer + bufferIndex,
        outputBuffer + bufferIndex, length);
    input += length * inputChannels;
    output += length * outputChannels;
    frameCount -= length;
    bufferIndex = 0;
  }
}

template <int FixedOutputChannels>
inline void routeByMode(const float* input, float* output, int inputChannels,
                        int outputChannels, float* inputBuffer,
                        const float* outputBuffer, size_t bufferSize,
                        size_t bufferIndex, size_t frameCount,
                        SecondOutputChannel mode) {
  switch (mode) {
    case SecondOutputChannel::Processed:
      return routeWrapped<FixedOutputChannels, SecondOutputChannel::Processed>(
          input, output, inputChannels, outputChannels, inputBuffer, outputBuffer,
          bufferSize, bufferIndex, frameCount);
    case SecondOutputChannel::Inverted:
      return routeWrapped<FixedOutputChannels, SecondOutputChannel::Inverted>(
          input, output, inputChannels, outputChannels, inputBuffer, outputBuffer,
          bufferSize, bufferIndex, frameCount);
    case SecondOutputChannel::Input:
      return routeWrapped<FixedOutputChannels, SecondOutputChannel::Input>(
          input, output, inputChannels, outputChannels, inputBuffer, outputBuffer,
          bufferSize, bufferIndex, frameCount);
  }
  // Preserve the second channel for invalid modes, as before.
  return routeWrapped<FixedOutputChannels, static_cast<SecondOutputChannel>(-1)>(
      input, output, inputChannels, outputChannels, inputBuffer, outputBuffer,
      bufferSize, bufferIndex, frameCount);
}

}  // namespace audio_channel_routing

inline void routeAudioFrames(const float* input, float* output,
                             int inputChannels, int outputChannels,
                             float* inputBuffer, const float* outputBuffer,
                             size_t bufferSize, size_t bufferIndex,
                             size_t frameCount, SecondOutputChannel mode) {
  using namespace audio_channel_routing;
  if (outputChannels == 1) {
    routeWrapped<1, SecondOutputChannel::Processed>(
        input, output, inputChannels, outputChannels, inputBuffer, outputBuffer,
        bufferSize, bufferIndex, frameCount);
  } else if (outputChannels == 2) {
    routeByMode<2>(input, output, inputChannels, outputChannels, inputBuffer,
                   outputBuffer, bufferSize, bufferIndex, frameCount, mode);
  } else {
    routeByMode<0>(input, output, inputChannels, outputChannels, inputBuffer,
                   outputBuffer, bufferSize, bufferIndex, frameCount, mode);
  }
}

#endif  // AUDIO_CHANNEL_ROUTING_H
