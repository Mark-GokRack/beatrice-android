#ifndef BEATRICE_PROCESSOR_H
#define BEATRICE_PROCESSOR_H

#include <common/model_config.h>
#include <common/processor_core.h>

#include <filesystem>
#include <memory>
#include <string>

#include "beatriceParameters.h"
#include "effectors/AudioEffector.hpp"

class BeatriceProcessor : public AudioEffector {
 public:
  explicit BeatriceProcessor(const std::string& toml_path);

  std::shared_ptr<beatrice::common::ProcessorCoreBase> createProcessorCore(
      int32_t sampleRate);
  void resetProcessorCore();

  std::u8string getModelName() const;
  std::u8string getModelDescription() const;
  int32_t getModelVersion() const;

  void setVoiceID(int32_t voiceID);
  std::u8string getVoiceName(int32_t voiceID) const;
  std::u8string getVoiceDescription(int32_t voiceID) const;
  std::u8string getVoicePortraitPath(int32_t voiceID) const;
  std::u8string getVoicePortraitDescription(int32_t voiceID) const;

  void setPitchShift(double pitchShift);
  void setFormantShift(double formantShift);
  void setInputGain(double gain);
  void setOutputGain(double gain);
  void setIntonationIntensity(double intensity);
  void setPitchCorrection(double correction);
  void setPitchCorrectionMode(int32_t mode);
  void setSourcePitchRange(double minPitch, double maxPitch);
  void setVQNumNeighbors(int32_t numNeighbors);
  void setSpeakerMorphingWeight(int32_t target_spk, double weight);

  BeatriceParameters getParameters() const;
  void setParameters(const BeatriceParameters& params);
  size_t getVoiceCount() const;

  // AudioEffector interface
  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;
  void setSampleRate(float sampleRate) override;
  void setEnabled(bool enabled) override;
  bool isEnabled() const override;

 private:
  void applyParametersToCore();
  bool isValidVoiceId(int32_t voiceID) const;

  std::shared_ptr<beatrice::common::ProcessorCoreBase> mBeatriceProcessorCore;
  beatrice::common::ModelConfig mBeatriceModelConfig;
  std::filesystem::path mBeatriceModelPath;
  BeatriceParameters mBeatriceParameters;
  size_t mBeatriceVoiceCount = 0;
  bool mIsEnabled = true;
};

#endif  // BEATRICE_PROCESSOR_H