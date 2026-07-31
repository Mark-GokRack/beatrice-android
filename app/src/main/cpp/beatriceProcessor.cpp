#include "beatriceProcessor.h"

#include <common/processor_core_0.h>
#include <common/processor_core_1.h>
#include <common/processor_core_2.h>
#include <logging_macros.h>

#include <stdexcept>

#include "toml11/single_include/toml.hpp"

using namespace beatrice::common;

namespace {
std::array<float, kMaxNSpeakers> zeroUnusedWeights(
    const std::array<float, kMaxNSpeakers>& weights, size_t voiceCount) {
  std::array<float, kMaxNSpeakers> result{};
  for (size_t i = 0; i < voiceCount; ++i) {
    result[i] = weights[i];
  }
  return result;
}
}  // namespace

BeatriceProcessor::BeatriceProcessor(const std::string& toml_path_str) {
  mBeatriceModelPath = std::filesystem::path(toml_path_str);
  const auto toml_data = toml::parse(mBeatriceModelPath);
  mBeatriceModelConfig = toml::get<beatrice::common::ModelConfig>(toml_data);

  for (auto i = 0; i < kMaxNSpeakers; ++i) {
    mBeatriceParameters.averageTargetPitchBase[i] =
        mBeatriceModelConfig.voices[i].average_pitch;
  }

  mBeatriceVoiceCount = kMaxNSpeakers;
  for (auto i = 0; i < kMaxNSpeakers; ++i) {
    const auto& voice = mBeatriceModelConfig.voices[i];
    if (voice.name.empty() && voice.description.empty() &&
        voice.portrait.path.empty() && voice.portrait.description.empty()) {
      mBeatriceVoiceCount = i;
      break;
    }
  }

  double morphed_average_pitch = 0.0;
  for (auto i = 0; i < mBeatriceVoiceCount; ++i) {
    morphed_average_pitch += mBeatriceModelConfig.voices[i].average_pitch;
  }
  if (mBeatriceVoiceCount > 0) {
    morphed_average_pitch /= static_cast<double>(mBeatriceVoiceCount);
  }
  mBeatriceParameters.averageTargetPitchBase[mBeatriceVoiceCount] =
      morphed_average_pitch;
}

std::shared_ptr<beatrice::common::ProcessorCoreBase>
BeatriceProcessor::createProcessorCore(int32_t sampleRate) {
  if (mBeatriceModelConfig.model.VersionInt() == 0) {
    mBeatriceProcessorCore =
        std::make_shared<beatrice::common::ProcessorCore0>(sampleRate);
  } else if (mBeatriceModelConfig.model.VersionInt() == 1) {
    mBeatriceProcessorCore =
        std::make_shared<beatrice::common::ProcessorCore1>(sampleRate);
  } else if (mBeatriceModelConfig.model.VersionInt() == 2) {
    mBeatriceProcessorCore =
        std::make_shared<beatrice::common::ProcessorCore2>(sampleRate);
  } else {
    mBeatriceProcessorCore =
        std::make_shared<beatrice::common::ProcessorCoreUnloaded>();
    throw std::runtime_error("Unsupported model version");
  }

  if (auto error_code = mBeatriceProcessorCore->LoadModel(mBeatriceModelConfig,
                                                          mBeatriceModelPath);
      error_code != beatrice::common::ErrorCode::kSuccess) {
    mBeatriceProcessorCore.reset();
    throw std::runtime_error("Failed to load model");
  }

  applyParametersToCore();
  return mBeatriceProcessorCore;
}

void BeatriceProcessor::resetProcessorCore() { mBeatriceProcessorCore.reset(); }

std::u8string BeatriceProcessor::getModelName() const {
  return mBeatriceModelConfig.model.name;
}

std::u8string BeatriceProcessor::getModelDescription() const {
  return mBeatriceModelConfig.model.description;
}

int32_t BeatriceProcessor::getModelVersion() const {
  return mBeatriceModelConfig.model.VersionInt();
}

void BeatriceProcessor::setVoiceID(int32_t voiceID) {
  if (!isValidVoiceId(voiceID)) {
    LOGW("Invalid voiceID: %d", voiceID);
    return;
  }
  mBeatriceParameters.targetSpeaker = voiceID;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetTargetSpeaker(mBeatriceParameters.targetSpeaker);
    mBeatriceProcessorCore->SetAverageSourcePitch(
        mBeatriceParameters
            .averageTargetPitchBase[mBeatriceParameters.targetSpeaker] -
        mBeatriceParameters.pitchShift);
  }
}

std::u8string BeatriceProcessor::getVoiceName(int32_t voiceID) const {
  if (!isValidVoiceId(voiceID)) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].name;
}

std::u8string BeatriceProcessor::getVoiceDescription(int32_t voiceID) const {
  if (!isValidVoiceId(voiceID)) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }

  std::u8string description = u8"";
  if (voiceID < static_cast<int32_t>(mBeatriceVoiceCount)) {
    description += u8"[" + mBeatriceModelConfig.voices[voiceID].name + u8"]\n";
    description += mBeatriceModelConfig.voices[voiceID].description;
  } else if (voiceID == static_cast<int32_t>(mBeatriceVoiceCount)) {
    description += u8"<< Voice Morphing Mode >>\n";
    description += u8"[注意 / Caution]";
    description += u8"\n";
    description +=
        u8"Voice Morphing Mode では、未選択の Voice の学習データが\n"
        u8"変換結果に影響を与えやすくなる可能性があります。\n"
        u8"意図せぬ声質の類似や権利侵害にご注意ください。\n";
    description +=
        u8"In Voice Morphing Mode, the training data of unselected Voices "
        u8"could "
        u8"be more prone to influencing the conversion results. Please be "
        u8"mindful of unintended similarities in timbre and possible rights "
        u8"infringement.\n";

    for (auto i = 0; i < mBeatriceVoiceCount; ++i) {
      if (mBeatriceParameters.speakerMorphingWeights[i] <= 0.0) {
        continue;
      }
      description += u8"\n";
      description += u8"[" + mBeatriceModelConfig.voices[i].name + u8"]\n";
      description += mBeatriceModelConfig.voices[i].description;
    }
  }
  return description;
}

std::u8string BeatriceProcessor::getVoicePortraitPath(int32_t voiceID) const {
  if (!isValidVoiceId(voiceID)) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }
  if (voiceID >= static_cast<int32_t>(mBeatriceVoiceCount)) {
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].portrait.path;
}

std::u8string BeatriceProcessor::getVoicePortraitDescription(
    int32_t voiceID) const {
  if (!isValidVoiceId(voiceID)) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }
  if (voiceID >= static_cast<int32_t>(mBeatriceVoiceCount)) {
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].portrait.description;
}

void BeatriceProcessor::setPitchShift(double pitchShift) {
  mBeatriceParameters.pitchShift = pitchShift;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchShift(mBeatriceParameters.pitchShift);
    mBeatriceProcessorCore->SetAverageSourcePitch(
        mBeatriceParameters
            .averageTargetPitchBase[mBeatriceParameters.targetSpeaker] -
        mBeatriceParameters.pitchShift);
  }
}

void BeatriceProcessor::setFormantShift(double formantShift) {
  mBeatriceParameters.formantShift = formantShift;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetFormantShift(mBeatriceParameters.formantShift);
  }
}

void BeatriceProcessor::setInputGain(double gain) {
  mBeatriceParameters.inputGain = gain;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetInputGain(mBeatriceParameters.inputGain);
  }
}

void BeatriceProcessor::setOutputGain(double gain) {
  mBeatriceParameters.outputGain = gain;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetOutputGain(mBeatriceParameters.outputGain);
  }
}

void BeatriceProcessor::setIntonationIntensity(double intensity) {
  mBeatriceParameters.intonationIntensity = intensity;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetIntonationIntensity(
        mBeatriceParameters.intonationIntensity);
  }
}

void BeatriceProcessor::setPitchCorrection(double correction) {
  mBeatriceParameters.pitchCorrection = correction;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchCorrection(
        mBeatriceParameters.pitchCorrection);
  }
}

void BeatriceProcessor::setPitchCorrectionMode(int32_t mode) {
  mBeatriceParameters.pitchCorrectionMode = mode;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchCorrectionType(
        mBeatriceParameters.pitchCorrectionMode);
  }
}

void BeatriceProcessor::setSourcePitchRange(double minPitch, double maxPitch) {
  mBeatriceParameters.minSourcePitch = minPitch;
  mBeatriceParameters.maxSourcePitch = maxPitch;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetMinSourcePitch(
        mBeatriceParameters.minSourcePitch);
    mBeatriceProcessorCore->SetMaxSourcePitch(
        mBeatriceParameters.maxSourcePitch);
  }
}

void BeatriceProcessor::setVQNumNeighbors(int32_t numNeighbors) {
  mBeatriceParameters.vqNumNeighbors = numNeighbors;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetVQNumNeighbors(
        mBeatriceParameters.vqNumNeighbors);
  }
}

bool BeatriceProcessor::setSpeakerMorphingWeights(
    const std::array<float, beatrice::common::kMaxNSpeakers>& weights) {
  const auto cleanWeights =
      zeroUnusedWeights(weights, static_cast<size_t>(mBeatriceVoiceCount));
  mBeatriceParameters.speakerMorphingWeights = cleanWeights;

  if (mBeatriceProcessorCore) {
    auto error_code =
        mBeatriceProcessorCore->SetSpeakerMorphingWeights(cleanWeights);
    if (error_code != ErrorCode::kSuccess) {
      LOGW("Failed to set speaker morphing weights: %d",
           static_cast<int>(error_code));
      return false;
    }
  }
  return true;
}

BeatriceParameters BeatriceProcessor::getParameters() const {
  return mBeatriceParameters;
}

void BeatriceProcessor::setParameters(const BeatriceParameters& params) {
  if (params.targetSpeaker >= 0 &&
      params.targetSpeaker <= static_cast<int32_t>(mBeatriceVoiceCount)) {
    setVoiceID(params.targetSpeaker);
  } else {
    setVoiceID(0);
  }
  setFormantShift(params.formantShift);
  setPitchShift(params.pitchShift);
  setInputGain(params.inputGain);
  setOutputGain(params.outputGain);
  setIntonationIntensity(params.intonationIntensity);
  setPitchCorrection(params.pitchCorrection);
  setPitchCorrectionMode(params.pitchCorrectionMode);
  setSourcePitchRange(params.minSourcePitch, params.maxSourcePitch);
  setVQNumNeighbors(params.vqNumNeighbors);
  setSpeakerMorphingWeights(params.speakerMorphingWeights);
}

size_t BeatriceProcessor::getVoiceCount() const { return mBeatriceVoiceCount; }

void BeatriceProcessor::applyParametersToCore() {
  if (!mBeatriceProcessorCore) {
    return;
  }

  mBeatriceProcessorCore->SetTargetSpeaker(mBeatriceParameters.targetSpeaker);
  mBeatriceProcessorCore->SetFormantShift(mBeatriceParameters.formantShift);
  mBeatriceProcessorCore->SetPitchShift(mBeatriceParameters.pitchShift);
  mBeatriceProcessorCore->SetInputGain(mBeatriceParameters.inputGain);
  mBeatriceProcessorCore->SetOutputGain(mBeatriceParameters.outputGain);
  mBeatriceProcessorCore->SetAverageSourcePitch(
      mBeatriceParameters
          .averageTargetPitchBase[mBeatriceParameters.targetSpeaker] -
      mBeatriceParameters.pitchShift);
  mBeatriceProcessorCore->SetIntonationIntensity(
      mBeatriceParameters.intonationIntensity);
  mBeatriceProcessorCore->SetPitchCorrection(
      mBeatriceParameters.pitchCorrection);
  mBeatriceProcessorCore->SetPitchCorrectionType(
      mBeatriceParameters.pitchCorrectionMode);
  mBeatriceProcessorCore->SetMinSourcePitch(mBeatriceParameters.minSourcePitch);
  mBeatriceProcessorCore->SetMaxSourcePitch(mBeatriceParameters.maxSourcePitch);
  mBeatriceProcessorCore->SetVQNumNeighbors(mBeatriceParameters.vqNumNeighbors);
  mBeatriceProcessorCore->SetSpeakerMorphingWeights(
      mBeatriceParameters.speakerMorphingWeights);
}

bool BeatriceProcessor::isValidVoiceId(int32_t voiceID) const {
  return voiceID >= 0 && voiceID <= beatrice::common::kMaxNSpeakers;
}

void BeatriceProcessor::process(const float* inputBuffer, float* outputBuffer,
                                int numSamples) {
  if (mBeatriceProcessorCore && mIsEnabled) {
    mBeatriceProcessorCore->Process(inputBuffer, outputBuffer, numSamples);
  } else {
    std::fill_n(outputBuffer, numSamples, 0.0f);
  }
}

void BeatriceProcessor::setSampleRate(float sampleRate) {
  createProcessorCore(static_cast<int32_t>(sampleRate));
}

void BeatriceProcessor::setEnabled(bool enabled) { mIsEnabled = enabled; }

bool BeatriceProcessor::isEnabled() const { return mIsEnabled; }
