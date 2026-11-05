#include "../../main/cpp/AudioChannelRouting.h"

#include <cassert>
#include <initializer_list>

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
}
