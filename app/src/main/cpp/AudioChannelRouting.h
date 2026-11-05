#ifndef AUDIO_CHANNEL_ROUTING_H
#define AUDIO_CHANNEL_ROUTING_H

#include <cstddef>

enum class SecondOutputChannel {
  Processed = 0,
  Inverted = 1,
  Input = 2,
};

inline void routeAudioFrames(const float* input, float* output,
                             int inputChannels, int outputChannels,
                             float* inputBuffer, const float* outputBuffer,
                             size_t bufferSize, size_t bufferIndex,
                             size_t frameCount, SecondOutputChannel mode) {
  for (size_t frame = 0; frame < frameCount; ++frame) {
    const size_t index = (bufferIndex + frame) % bufferSize;
    const float inputSample = input[frame * inputChannels];
    const float processedSample = outputBuffer[index];
    inputBuffer[index] = inputSample;
    output[frame * outputChannels] = processedSample;
    if (outputChannels > 1) {
      switch (mode) {
        case SecondOutputChannel::Processed:
          output[frame * outputChannels + 1] = processedSample;
          break;
        case SecondOutputChannel::Inverted:
          output[frame * outputChannels + 1] = -processedSample;
          break;
        case SecondOutputChannel::Input:
          output[frame * outputChannels + 1] = inputSample;
          break;
      }
    }
    for (int channel = 2; channel < outputChannels; ++channel) {
      output[frame * outputChannels + channel] = 0.0f;
    }
  }
}

#endif  // AUDIO_CHANNEL_ROUTING_H
