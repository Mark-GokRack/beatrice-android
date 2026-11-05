#include "../../main/cpp/AudioChannelRouting.h"

#include <cassert>
#include <cstring>
#include <initializer_list>
#include <vector>

static void checkRouting(int inputChannels, int outputChannels,
                         SecondOutputChannel mode, size_t bufferIndex,
                         size_t frameCount) {
  constexpr size_t bufferSize = 7;
  std::vector<float> input(frameCount * inputChannels);
  std::vector<float> output(frameCount * outputChannels, 123.0f);
  std::vector<float> expectedOutput = output;
  std::vector<float> captured(bufferSize, -123.0f);
  std::vector<float> expectedCaptured = captured;
  const float processed[bufferSize] = {0.0f, -0.5f, 0.75f, -1.0f,
                                       1.25f, -1.5f, 1.75f};
  for (size_t i = 0; i < input.size(); ++i) {
    input[i] = static_cast<float>(i + 1);
  }
  for (size_t frame = 0; frame < frameCount; ++frame) {
    const size_t index = (bufferIndex + frame) % bufferSize;
    const float inputSample = input[frame * inputChannels];
    const float processedSample = processed[index];
    expectedCaptured[index] = inputSample;
    expectedOutput[frame * outputChannels] = processedSample;
    if (outputChannels > 1) {
      switch (mode) {
        case SecondOutputChannel::Processed:
          expectedOutput[frame * outputChannels + 1] = processedSample;
          break;
        case SecondOutputChannel::Inverted:
          expectedOutput[frame * outputChannels + 1] = -processedSample;
          break;
        case SecondOutputChannel::Input:
          expectedOutput[frame * outputChannels + 1] = inputSample;
          break;
      }
    }
    for (int channel = 2; channel < outputChannels; ++channel) {
      expectedOutput[frame * outputChannels + channel] = 0.0f;
    }
  }
  routeAudioFrames(input.data(), output.data(), inputChannels, outputChannels,
                   captured.data(), processed, bufferSize, bufferIndex,
                   frameCount, mode);
  assert(output.empty() ||
         std::memcmp(output.data(), expectedOutput.data(),
                     output.size() * sizeof(float)) == 0);
  assert(captured == expectedCaptured);
}

int main() {
  const float input[] = {1.0f, 99.0f, -2.0f, 99.0f, 3.0f, 99.0f};
  const float processed[] = {0.25f, -0.5f, 0.75f, -1.0f};

  for (auto mode : {SecondOutputChannel::Processed,
                    SecondOutputChannel::Inverted,
                    SecondOutputChannel::Input}) {
    float captured[4] = {};
    float stereo[6] = {};
    routeAudioFrames(input, stereo, 2, 2, captured, processed, 4, 3, 3, mode);
    const float expectedProcessed[] = {-1.0f, 0.25f, -0.5f};
    const float expectedInput[] = {1.0f, -2.0f, 3.0f};
    for (int i = 0; i < 3; ++i) {
      assert(stereo[2 * i] == expectedProcessed[i]);
      const float expectedSecond = mode == SecondOutputChannel::Processed
                                       ? expectedProcessed[i]
                                       : mode == SecondOutputChannel::Inverted
                                             ? -expectedProcessed[i]
                                             : expectedInput[i];
      assert(stereo[2 * i + 1] == expectedSecond);
      assert(captured[(3 + i) % 4] == expectedInput[i]);
    }

    float mono[3] = {};
    routeAudioFrames(input, mono, 2, 1, captured, processed, 4, 3, 3, mode);
    for (int i = 0; i < 3; ++i) {
      assert(mono[i] == expectedProcessed[i]);
    }
  }

  const float monoInput[] = {4.0f, 5.0f};
  float captured[4] = {};
  float multichannel[6] = {};
  routeAudioFrames(monoInput, multichannel, 1, 3, captured, processed, 4, 0, 2,
                   SecondOutputChannel::Input);
  assert(multichannel[0] == 0.25f && multichannel[1] == 4.0f &&
         multichannel[2] == 0.0f);
  assert(multichannel[3] == -0.5f && multichannel[4] == 5.0f &&
         multichannel[5] == 0.0f);

  for (int inputChannels : {1, 2, 4}) {
    for (int outputChannels : {1, 2, 3, 5}) {
      for (auto mode : {SecondOutputChannel::Processed,
                        SecondOutputChannel::Inverted,
                        SecondOutputChannel::Input}) {
        for (size_t bufferIndex : {size_t{0}, size_t{3}, size_t{6}}) {
          for (size_t frameCount : {size_t{0}, size_t{1}, size_t{7},
                                    size_t{8}, size_t{20}}) {
            checkRouting(inputChannels, outputChannels, mode, bufferIndex,
                         frameCount);
          }
        }
      }
    }
  }
}
