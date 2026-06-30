#include "beatriceEngine.h"

#include <common/processor_core_0.h>
#include <common/processor_core_1.h>
#include <common/processor_core_2.h>
#include <logging_macros.h>
#include <oboe/LatencyTuner.h>

#include <cassert>

#include "toml11/single_include/toml.hpp"

using namespace beatrice::common;

beatriceEngine::beatriceEngine(const std::string& toml_path_str) {
  assert(mOutputChannelCount == mInputChannelCount);
  mBeatriceModelPath = std::filesystem::path(toml_path_str);
  const auto toml_data = toml::parse(mBeatriceModelPath);
  mBeatriceModelConfig = toml::get<beatrice::common::ModelConfig>(toml_data);

  // AverageTargetPitches
  for (auto i = 0; i < kMaxNSpeakers; ++i) {
    mBeatriceParameters.averageTargetPitchBase[i] =
        mBeatriceModelConfig.voices[i].average_pitch;
  }

  // Voice Morph の AverageTargetPitch を計算
  // 今のところは各 Voice の値の単純平均を採用することとする
  auto voice_counter = kMaxNSpeakers;
  for (auto i = 0; i < kMaxNSpeakers; ++i) {
    const auto& voice = mBeatriceModelConfig.voices[i];
    if (voice.name.empty() && voice.description.empty() &&
        voice.portrait.path.empty() && voice.portrait.description.empty()) {
      voice_counter = i;
      break;
    }
  }
  double morphed_average_pitch = 0;
  for (auto i = 0; i < voice_counter; ++i) {
    morphed_average_pitch += mBeatriceModelConfig.voices[i].average_pitch;
  }
  morphed_average_pitch /= voice_counter;
  mBeatriceParameters.averageTargetPitchBase[kMaxNSpeakers] =
      morphed_average_pitch;
}

void beatriceEngine::setRecordingDeviceId(int32_t deviceId) {
  mRecordingDeviceId = deviceId;
}

void beatriceEngine::setPerformanceMode(oboe::PerformanceMode mode) {
  mPerformanceMode = mode;
}

void beatriceEngine::setAsyncMode(bool isAsyncMode) {
  mIsAsyncMode = isAsyncMode;
}

void beatriceEngine::setPlaybackDeviceId(int32_t deviceId) {
  mPlaybackDeviceId = deviceId;
}

bool beatriceEngine::isAAudioRecommended() {
  return oboe::AudioStreamBuilder::isAAudioRecommended();
}

bool beatriceEngine::setAudioApi(oboe::AudioApi api) {
  if (mIsEffectOn) return false;
  mAudioApi = api;
  return true;
}

bool beatriceEngine::setEffectOn(bool isOn) {
  bool success = true;
  if (isOn != mIsEffectOn) {
    if (isOn) {
      success = openStreams() == oboe::Result::OK;
      if (success) {
        mIsEffectOn = isOn;
      }
    } else {
      closeStreams();
      mIsEffectOn = isOn;
    }
  }
  return success;
}

void beatriceEngine::closeStreams() {
  mDuplexStream->stop();
  closeStream(mPlayStream);
  closeStream(mRecordingStream);
  mDuplexStream.reset();
}

oboe::Result beatriceEngine::openStreams() {
  oboe::AudioStreamBuilder inBuilder, outBuilder;

  // First, open playback stream with optimal settings to detect its sample
  // rate.
  setupPlaybackStreamParameters(&outBuilder);
  oboe::Result result = outBuilder.openStream(mPlayStream);
  if (result != oboe::Result::OK) {
    LOGE("Failed to open output stream for rate detection. Error %s",
         oboe::convertToText(result));
    return result;
  }
  int32_t rateP = mPlayStream->getSampleRate();

  // Next, open recording stream with unspecified sample rate to detect its
  // optimal rate.
  setupRecordingStreamParameters(&inBuilder, oboe::kUnspecified);
  result = inBuilder.openStream(mRecordingStream);
  if (result != oboe::Result::OK) {
    LOGE("Failed to open input stream for rate detection. Error %s",
         oboe::convertToText(result));
    closeStream(mPlayStream);
    return result;
  }
  int32_t rateR = mRecordingStream->getSampleRate();

  // Compare rates and use the lower one to ensure compatibility (especially for
  // Bluetooth).
  if (rateP == rateR) {
    mSampleRate = rateP;
    LOGI("Playback and Recording rates match: %d Hz", mSampleRate);
  } else {
    mSampleRate = std::min(rateP, rateR);
    LOGI("Sample rates differ (P:%d, R:%d). Using lower rate: %d Hz", rateP,
         rateR, mSampleRate);

    // Close and reopen both streams with the common sample rate.
    closeStream(mPlayStream);
    closeStream(mRecordingStream);

    setupPlaybackStreamParameters(&outBuilder);
    outBuilder.setSampleRate(mSampleRate);
    result = outBuilder.openStream(mPlayStream);
    if (result != oboe::Result::OK) {
      LOGE("Failed to re-open output stream with common rate. Error %s",
           oboe::convertToText(result));
      return result;
    }

    setupRecordingStreamParameters(&inBuilder, mSampleRate);
    // Use the playback stream's capacity as a reference for recording buffer
    // capacity.
    inBuilder.setBufferCapacityInFrames(
        mPlayStream->getBufferCapacityInFrames() * 2);
    result = inBuilder.openStream(mRecordingStream);
    if (result != oboe::Result::OK) {
      LOGE("Failed to re-open input stream with common rate. Error %s",
           oboe::convertToText(result));
      closeStream(mPlayStream);
      return result;
    }
  }

  mLatencyTuner = std::make_shared<oboe::LatencyTuner>(*mPlayStream);

  if (mBeatriceModelConfig.model.VersionInt() == 0) {
    mBeatriceProcessorCore =
        std::make_unique<beatrice::common::ProcessorCore0>(mSampleRate);
  } else if (mBeatriceModelConfig.model.VersionInt() == 1) {
    mBeatriceProcessorCore =
        std::make_unique<beatrice::common::ProcessorCore1>(mSampleRate);
  } else if (mBeatriceModelConfig.model.VersionInt() == 2) {
    mBeatriceProcessorCore =
        std::make_unique<beatrice::common::ProcessorCore2>(mSampleRate);
  } else {
    mBeatriceProcessorCore =
        std::make_unique<beatrice::common::ProcessorCoreUnloaded>();
    throw std::runtime_error("Unsupported model version");
  }
  if (auto error_code = mBeatriceProcessorCore->LoadModel(mBeatriceModelConfig,
                                                          mBeatriceModelPath);
      error_code != beatrice::common::ErrorCode::kSuccess) {
    throw std::runtime_error("Failed to load model");
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

  mDuplexStream = std::make_unique<BeatriceFullDuplexPass>(
      mBeatriceProcessorCore, mLatencyTuner, mIsAsyncMode, 480, 2);
  mDuplexStream->setSharedInputStream(mRecordingStream);
  mDuplexStream->setSharedOutputStream(mPlayStream);
  mDuplexStream->start();
  return result;
}

oboe::AudioStreamBuilder* beatriceEngine::setupRecordingStreamParameters(
    oboe::AudioStreamBuilder* builder, int32_t sampleRate) {
  builder->setDeviceId(mRecordingDeviceId)
      ->setDirection(oboe::Direction::Input)
      ->setSampleRate(sampleRate)
      ->setChannelCount(mInputChannelCount);

  return setupCommonStreamParameters(builder);
}

oboe::AudioStreamBuilder* beatriceEngine::setupPlaybackStreamParameters(
    oboe::AudioStreamBuilder* builder) {
  builder->setDataCallback(this)
      ->setErrorCallback(this)
      ->setDeviceId(mPlaybackDeviceId)
      ->setDirection(oboe::Direction::Output)
      ->setChannelCount(mOutputChannelCount);

  return setupCommonStreamParameters(builder);
}

oboe::AudioStreamBuilder* beatriceEngine::setupCommonStreamParameters(
    oboe::AudioStreamBuilder* builder) {
  builder->setAudioApi(mAudioApi)
      ->setFormat(mFormat)
      ->setFormatConversionAllowed(true)
      ->setSharingMode(oboe::SharingMode::Exclusive);
  if (mAudioApi == oboe::AudioApi::AAudio) {
    builder->setPerformanceMode(mPerformanceMode);
  } else {
    builder->setPerformanceMode(oboe::PerformanceMode::None);
  }
  // builder->setFramesPerDataCallback(480);
  builder->setUsage(oboe::Usage::Game);
  return builder;
}

void beatriceEngine::closeStream(std::shared_ptr<oboe::AudioStream>& stream) {
  if (stream) {
    oboe::Result result = stream->stop();
    if (result != oboe::Result::OK) {
      LOGW("Error stopping stream: %s", oboe::convertToText(result));
    }
    result = stream->close();
    if (result != oboe::Result::OK) {
      LOGE("Error closing stream: %s", oboe::convertToText(result));
    } else {
      LOGW("Successfully closed streams");
    }
    stream.reset();
  }
  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore.reset();
  }
}

void beatriceEngine::warnIfNotLowLatency(
    std::shared_ptr<oboe::AudioStream>& stream) {
  if (stream->getPerformanceMode() != oboe::PerformanceMode::LowLatency) {
    LOGW(
        "Stream is NOT low latency."
        "Check your requested format, sample rate and channel count");
  }
}

oboe::DataCallbackResult beatriceEngine::onAudioReady(
    oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames) {
  return mDuplexStream->onAudioReady(oboeStream, audioData, numFrames);
}

void beatriceEngine::onErrorBeforeClose(oboe::AudioStream* oboeStream,
                                        oboe::Result error) {
  LOGE("%s stream Error before close: %s",
       oboe::convertToText(oboeStream->getDirection()),
       oboe::convertToText(error));
}

void beatriceEngine::onErrorAfterClose(oboe::AudioStream* oboeStream,
                                       oboe::Result error) {
  LOGE("%s stream Error after close: %s",
       oboe::convertToText(oboeStream->getDirection()),
       oboe::convertToText(error));

  closeStreams();

  if (error == oboe::Result::ErrorDisconnected) {
    LOGI("Restarting AudioStream");
    openStreams();
  }
}

void beatriceEngine::setVoiceID(int32_t voiceID) {
  if (voiceID < 0 || voiceID > beatrice::common::kMaxNSpeakers) {
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

std::u8string beatriceEngine::getModelName(void) {
  return mBeatriceModelConfig.model.name;
}

std::u8string beatriceEngine::getModelDescription(void) {
  return mBeatriceModelConfig.model.description;
}

int32_t beatriceEngine::getModelVersion(void) {
  return mBeatriceModelConfig.model.VersionInt();
}

std::u8string beatriceEngine::getVoiceName(int32_t voiceID) {
  if (voiceID < 0 || voiceID > beatrice::common::kMaxNSpeakers) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].name;
}

std::u8string beatriceEngine::getVoiceDescription(int32_t voiceID) {
  if (voiceID < 0 || voiceID > beatrice::common::kMaxNSpeakers) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }
  if (voiceID == static_cast<int32_t>(mBeatriceModelConfig.voices.size())) {
    std::u8string description = u8"Voice Morphing Mode: \n";
    for (auto i = 0; i < mBeatriceModelConfig.voices.size(); ++i) {
      if (mBeatriceParameters.speakerMorphingWeights[i] <= 0.0) {
        continue;
      }
      description += mBeatriceModelConfig.voices[i].description + u8"\n";
    }
    return description;
  }
  if (voiceID > static_cast<int32_t>(mBeatriceModelConfig.voices.size())) {
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].description;
}

std::u8string beatriceEngine::getVoicePortraitPath(int32_t voiceID) {
  if (voiceID < 0 || voiceID > beatrice::common::kMaxNSpeakers) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }
  if (voiceID >= static_cast<int32_t>(mBeatriceModelConfig.voices.size())) {
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].portrait.path;
}

std::u8string beatriceEngine::getVoicePortraitDescription(int32_t voiceID) {
  if (voiceID < 0 || voiceID > beatrice::common::kMaxNSpeakers) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }
  if (voiceID >= static_cast<int32_t>(mBeatriceModelConfig.voices.size())) {
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].portrait.description;
}

void beatriceEngine::setPitchShift(double pitchShift) {
  mBeatriceParameters.pitchShift = pitchShift;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchShift(mBeatriceParameters.pitchShift);
    mBeatriceProcessorCore->SetAverageSourcePitch(
        mBeatriceParameters
            .averageTargetPitchBase[mBeatriceParameters.targetSpeaker] -
        mBeatriceParameters.pitchShift);
  }
}

void beatriceEngine::setFormantShift(double formantShift) {
  mBeatriceParameters.formantShift = formantShift;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetFormantShift(mBeatriceParameters.formantShift);
  }
}

void beatriceEngine::setInputGain(double gain) {
  mBeatriceParameters.inputGain = gain;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetInputGain(mBeatriceParameters.inputGain);
  }
}

void beatriceEngine::setOutputGain(double gain) {
  mBeatriceParameters.outputGain = gain;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetOutputGain(mBeatriceParameters.outputGain);
  }
}

void beatriceEngine::setIntonationIntensity(double intensity) {
  mBeatriceParameters.intonationIntensity = intensity;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetIntonationIntensity(
        mBeatriceParameters.intonationIntensity);
  }
}

void beatriceEngine::setPitchCorrection(double correction) {
  mBeatriceParameters.pitchCorrection = correction;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchCorrection(
        mBeatriceParameters.pitchCorrection);
  }
}

void beatriceEngine::setPitchCorrectionMode(int32_t mode) {
  mBeatriceParameters.pitchCorrectionMode = mode;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchCorrectionType(
        mBeatriceParameters.pitchCorrectionMode);
  }
}

void beatriceEngine::setSourcePitchRange(double minPitch, double maxPitch) {
  mBeatriceParameters.minSourcePitch = minPitch;
  mBeatriceParameters.maxSourcePitch = maxPitch;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetMinSourcePitch(
        mBeatriceParameters.minSourcePitch);
    mBeatriceProcessorCore->SetMaxSourcePitch(
        mBeatriceParameters.maxSourcePitch);
  }
}

void beatriceEngine::setVQNumNeighbors(int32_t numNeighbors) {
  mBeatriceParameters.vqNumNeighbors = numNeighbors;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetVQNumNeighbors(
        mBeatriceParameters.vqNumNeighbors);
  }
}

void beatriceEngine::setSpeakerMorphingWeight(int32_t target_spk,
                                              double weight) {
  if (target_spk < 0 || target_spk >= beatrice::common::kMaxNSpeakers) {
    LOGW("Invalid target_spk: %d", target_spk);
    return;
  }
  mBeatriceParameters.speakerMorphingWeights[target_spk] = weight;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetSpeakerMorphingWeight(target_spk, weight);
  }
}

BeatriceParameters beatriceEngine::getParameters() const {
  return mBeatriceParameters;
}

void beatriceEngine::setParameters(const BeatriceParameters& params) {
  if (params.targetSpeaker >= 0 &&
      params.targetSpeaker <= mBeatriceModelConfig.voices.size()) {
    // == mBeatriceModelConfig.voices.size() は Voice Morph 用
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
  for (int32_t i = 0; i < beatrice::common::kMaxNSpeakers; ++i) {
    setSpeakerMorphingWeight(i, params.speakerMorphingWeights[i]);
  }
}

int32_t beatriceEngine::getSampleRate() const { return mSampleRate; }

int32_t beatriceEngine::getFramesPerBurst() const {
  if (mPlayStream) {
    return mPlayStream->getFramesPerBurst();
  }
  return 0;
}