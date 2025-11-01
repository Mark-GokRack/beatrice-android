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
  setupPlaybackStreamParameters(&outBuilder);
  oboe::Result result = outBuilder.openStream(mPlayStream);
  // auto outBurstSize = mPlayStream->getFramesPerBurst();
  // mPlayStream->setBufferSizeInFrames(std::max(480 * 2, 2 * outBurstSize));

  mLatencyTuner = std::make_shared<oboe::LatencyTuner>(*mPlayStream);

  // auto outBufferSize = mPlayStream->getBufferSizeInFrames();
  // auto outBufferCapacity = mPlayStream->getBufferCapacityInFrames();

  if (result != oboe::Result::OK) {
    LOGE("Failed to open output stream. Error %s", oboe::convertToText(result));
    mSampleRate = oboe::kUnspecified;
    return result;
  } else {
    mSampleRate = mPlayStream->getSampleRate();
  }
  // warnIfNotLowLatency(mPlayStream);

  setupRecordingStreamParameters(&inBuilder, mSampleRate);
  inBuilder.setBufferCapacityInFrames(outBuilder.getBufferCapacityInFrames() *
                                      2);
  result = inBuilder.openStream(mRecordingStream);

  // auto inBurstSize = mRecordingStream->getFramesPerBurst();
  // auto inBufferSize = mRecordingStream->getBufferSizeInFrames();
  // auto inBufferCapacity = mRecordingStream->getBufferCapacityInFrames();

  if (result != oboe::Result::OK) {
    LOGE("Failed to open input stream. Error %s", oboe::convertToText(result));
    closeStream(mPlayStream);
    return result;
  }
  // warnIfNotLowLatency(mRecordingStream);

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

std::u8string beatriceEngine::getVoiceName(int32_t voiceID) {
  if (voiceID < 0 || voiceID > beatrice::common::kMaxNSpeakers) {
    LOGW("Invalid voiceID: %d", voiceID);
    return u8"";
  }

  return mBeatriceModelConfig.voices[voiceID].name;
}

void beatriceEngine::setPitchShift(float pitchShift) {
  mBeatriceParameters.pitchShift = pitchShift;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetPitchShift(mBeatriceParameters.pitchShift);
    mBeatriceProcessorCore->SetAverageSourcePitch(
        mBeatriceParameters
            .averageTargetPitchBase[mBeatriceParameters.targetSpeaker] -
        mBeatriceParameters.pitchShift);
  }
}

void beatriceEngine::setFormantShift(float formantShift) {
  mBeatriceParameters.formantShift = formantShift;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetFormantShift(mBeatriceParameters.formantShift);
  }
}

void beatriceEngine::setInputGain(float gain) {
  mBeatriceParameters.inputGain = gain;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetInputGain(mBeatriceParameters.inputGain);
  }
}

void beatriceEngine::setOutputGain(float gain) {
  mBeatriceParameters.outputGain = gain;

  if (mBeatriceProcessorCore) {
    mBeatriceProcessorCore->SetOutputGain(mBeatriceParameters.outputGain);
  }
}
