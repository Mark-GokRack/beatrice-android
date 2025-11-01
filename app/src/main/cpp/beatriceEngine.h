#ifndef BEATRICE_ENGINE_H
#define BEATRICE_ENGINE_H

#include <common/model_config.h>
#include <common/processor_core.h>
#include <jni.h>
#include <oboe/Oboe.h>

#include <filesystem>
#include <string>
#include <thread>

#include "beatriceFullDuplexPass.h"
#include "beatriceParameters.h"

class beatriceEngine : public oboe::AudioStreamCallback {
 public:
  beatriceEngine(const std::string& toml_path);

  void setPlaybackDeviceId(int32_t deviceId);
  void setRecordingDeviceId(int32_t deviceId);

  bool setEffectOn(bool isOn);

  bool setAudioApi(oboe::AudioApi);
  bool isAAudioRecommended(void);

  void setPerformanceMode(oboe::PerformanceMode mode);
  void setAsyncMode(bool isAsyncMode);

  void setVoiceID(int32_t voiceID);
  std::u8string getVoiceName(int32_t voiceID);

  void setPitchShift(float pitchShift);
  void setFormantShift(float formantShift);

  void setInputGain(float gain);
  void setOutputGain(float gain);

  oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboeStream,
                                        void* audioData,
                                        int32_t numFrames) override;

  void onErrorBeforeClose(oboe::AudioStream* oboeStream,
                          oboe::Result error) override;
  void onErrorAfterClose(oboe::AudioStream* oboeStream,
                         oboe::Result error) override;

 private:
  bool mIsEffectOn = false;
  int32_t mRecordingDeviceId = oboe::kUnspecified;
  int32_t mPlaybackDeviceId = oboe::kUnspecified;
  const oboe::AudioFormat mFormat = oboe::AudioFormat::Float;
  oboe::AudioApi mAudioApi = oboe::AudioApi::AAudio;
  int32_t mSampleRate = oboe::kUnspecified;
  const int32_t mInputChannelCount = oboe::ChannelCount::Mono;
  const int32_t mOutputChannelCount = oboe::ChannelCount::Mono;
  oboe::PerformanceMode mPerformanceMode = oboe::PerformanceMode::LowLatency;
  bool mIsAsyncMode = false;

  std::unique_ptr<BeatriceFullDuplexPass> mDuplexStream;
  std::shared_ptr<oboe::AudioStream> mRecordingStream;
  std::shared_ptr<oboe::AudioStream> mPlayStream;
  std::shared_ptr<oboe::LatencyTuner> mLatencyTuner;

  oboe::Result openStreams();

  void closeStreams();

  void closeStream(std::shared_ptr<oboe::AudioStream>& stream);

  oboe::AudioStreamBuilder* setupCommonStreamParameters(
      oboe::AudioStreamBuilder* builder);
  oboe::AudioStreamBuilder* setupRecordingStreamParameters(
      oboe::AudioStreamBuilder* builder, int32_t sampleRate);
  oboe::AudioStreamBuilder* setupPlaybackStreamParameters(
      oboe::AudioStreamBuilder* builder);
  void warnIfNotLowLatency(std::shared_ptr<oboe::AudioStream>& stream);

  std::shared_ptr<beatrice::common::ProcessorCoreBase> mBeatriceProcessorCore;
  beatrice::common::ModelConfig mBeatriceModelConfig;
  std::filesystem::path mBeatriceModelPath;
  BeatriceParameters mBeatriceParameters;
};

#endif  // BEATRICE_ENGINE_H
