#ifndef BEATRICE_PARAMETERS_H
#define BEATRICE_PARAMETERS_H

#include <common/model_config.h>

#include <array>

struct BeatriceParameters {
  int targetSpeaker = 0;
  double formantShift = 0.0;
  double pitchShift = 0.0;
  double inputGain = 0.0;
  double outputGain = 0.0;
  double averageSourcePitch = 52.0;
  double intonationIntensity = 1.0;
  double pitchCorrection = 0.0;
  int pitchCorrectionMode = 0;
  double minSourcePitch = 33.125;
  double maxSourcePitch = 80.875;
  int vqNumNeighbors = 4;
  std::array<double, beatrice::common::kMaxNSpeakers + 1>
      averageTargetPitchBase = {0.0};
  std::array<double, beatrice::common::kMaxNSpeakers> speakerMorphingWeights = {
      0.0};
};

#endif  // BEATRICE_PARAMETERS_H