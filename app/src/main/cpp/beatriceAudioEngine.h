#ifndef BEATRICE_AUDIO_ENGINE_H
#define BEATRICE_AUDIO_ENGINE_H

#include <oboe/LatencyTuner.h>
#include <oboe/Oboe.h>

#include <functional>
#include <memory>

#include "beatriceFullDuplexPass.h"
#include "effectors/AudioEffector.hpp"

class BeatriceAudioEngine : public oboe::AudioStreamCallback {
 public:
  void setPlaybackDeviceId(int32_t deviceId);
  void setRecordingDeviceId(int32_t deviceId);
  void setVoiceCommunicationMode(bool isVoiceCommunicationMode);
  void setPerformanceMode(oboe::PerformanceMode mode);
  void setAsyncMode(bool isAsyncMode);

  bool isAAudioRecommended() const;
  bool setAudioApi(oboe::AudioApi api);
  bool setEffectOn(bool isOn,
                   std::shared_ptr<AudioEffector> audioEffector = nullptr);

  int32_t getSampleRate() const;
  int32_t getFramesPerBurst() const;

  oboe::DataCallbackResult onAudioReady(oboe::AudioStream* oboeStream,
                                        void* audioData,
                                        int32_t numFrames) override;
  void onErrorBeforeClose(oboe::AudioStream* oboeStream,
                          oboe::Result error) override;
  void onErrorAfterClose(oboe::AudioStream* oboeStream,
                         oboe::Result error) override;

  oboe::Result openStreams(
      std::shared_ptr<AudioEffector> audioEffector = nullptr);
  void closeStreams();

 private:
  oboe::AudioStreamBuilder* setupCommonStreamParameters(
      oboe::AudioStreamBuilder* builder);
  oboe::AudioStreamBuilder* setupRecordingStreamParameters(
      oboe::AudioStreamBuilder* builder,
      int32_t sampleRate = oboe::kUnspecified);
  oboe::AudioStreamBuilder* setupPlaybackStreamParameters(
      oboe::AudioStreamBuilder* builder,
      int32_t sampleRate = oboe::kUnspecified);
  void closeStream(std::shared_ptr<oboe::AudioStream>& stream);
  void warnIfNotLowLatency(std::shared_ptr<oboe::AudioStream>& stream);

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
  bool mIsVoiceCommunicationMode = false;

  std::unique_ptr<BeatriceFullDuplexPass> mDuplexStream;
  std::shared_ptr<oboe::AudioStream> mRecordingStream;
  std::shared_ptr<oboe::AudioStream> mPlayStream;
  std::shared_ptr<oboe::LatencyTuner> mLatencyTuner;
  std::shared_ptr<AudioEffector> mAudioEffector;
};

#endif  // BEATRICE_AUDIO_ENGINE_H