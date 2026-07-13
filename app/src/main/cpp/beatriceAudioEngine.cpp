#include "beatriceAudioEngine.h"

#include <logging_macros.h>

#include <algorithm>
#include <exception>

void BeatriceAudioEngine::setPlaybackDeviceId(int32_t deviceId) {
  mPlaybackDeviceId = deviceId;
}

void BeatriceAudioEngine::setRecordingDeviceId(int32_t deviceId) {
  mRecordingDeviceId = deviceId;
}

void BeatriceAudioEngine::setPerformanceMode(oboe::PerformanceMode mode) {
  mPerformanceMode = mode;
}

void BeatriceAudioEngine::setAsyncMode(bool isAsyncMode) {
  mIsAsyncMode = isAsyncMode;
}

bool BeatriceAudioEngine::isAAudioRecommended() const {
  return oboe::AudioStreamBuilder::isAAudioRecommended();
}

bool BeatriceAudioEngine::setAudioApi(oboe::AudioApi api) {
  if (mIsEffectOn) {
    return false;
  }
  mAudioApi = api;
  return true;
}

bool BeatriceAudioEngine::setEffectOn(
    bool isOn, const ProcessorFactory& processorFactory) {
  bool success = true;
  if (isOn != mIsEffectOn) {
    if (isOn) {
      success = openStreams(processorFactory) == oboe::Result::OK;
      if (success) {
        mIsEffectOn = true;
      }
    } else {
      closeStreams();
      mIsEffectOn = false;
    }
  }
  return success;
}

oboe::Result BeatriceAudioEngine::openStreams(
    const ProcessorFactory& processorFactory) {
  mProcessorFactory = processorFactory;

  oboe::AudioStreamBuilder inBuilder, outBuilder;

  setupPlaybackStreamParameters(&outBuilder);
  oboe::Result result = outBuilder.openStream(mPlayStream);
  if (result != oboe::Result::OK) {
    LOGE("Failed to open output stream for rate detection. Error %s",
         oboe::convertToText(result));
    return result;
  }
  int32_t rateP = mPlayStream->getSampleRate();

  setupRecordingStreamParameters(&inBuilder, oboe::kUnspecified);
  result = inBuilder.openStream(mRecordingStream);
  if (result != oboe::Result::OK) {
    LOGE("Failed to open input stream for rate detection. Error %s",
         oboe::convertToText(result));
    closeStream(mPlayStream);
    return result;
  }
  int32_t rateR = mRecordingStream->getSampleRate();

  if (rateP == rateR) {
    mSampleRate = rateP;
    LOGI("Playback and Recording rates match: %d Hz", mSampleRate);
  } else {
    mSampleRate = std::min(rateP, rateR);
    LOGI("Sample rates differ (P:%d, R:%d). Using lower rate: %d Hz", rateP,
         rateR, mSampleRate);

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

  try {
    mProcessorCore = mProcessorFactory ? mProcessorFactory(mSampleRate) : nullptr;
  } catch (const std::exception& e) {
    LOGE("Failed to create processor core: %s", e.what());
    closeStreams();
    return oboe::Result::ErrorInternal;
  }

  if (!mProcessorCore) {
    LOGE("Processor factory returned null");
    closeStreams();
    return oboe::Result::ErrorInternal;
  }

  mLatencyTuner = std::make_shared<oboe::LatencyTuner>(*mPlayStream);
  mDuplexStream = std::make_unique<BeatriceFullDuplexPass>(
      mProcessorCore, mLatencyTuner, mIsAsyncMode, 480, 2);
  mDuplexStream->setSharedInputStream(mRecordingStream);
  mDuplexStream->setSharedOutputStream(mPlayStream);
  mDuplexStream->start();
  warnIfNotLowLatency(mPlayStream);

  return result;
}

void BeatriceAudioEngine::closeStreams() {
  if (mDuplexStream) {
    mDuplexStream->stop();
  }
  closeStream(mPlayStream);
  closeStream(mRecordingStream);
  mDuplexStream.reset();
  mLatencyTuner.reset();
  mProcessorCore.reset();
}

oboe::AudioStreamBuilder* BeatriceAudioEngine::setupRecordingStreamParameters(
    oboe::AudioStreamBuilder* builder, int32_t sampleRate) {
  builder->setDeviceId(mRecordingDeviceId)
      ->setDirection(oboe::Direction::Input)
      ->setSampleRate(sampleRate)
      ->setChannelCount(mInputChannelCount);

  return setupCommonStreamParameters(builder);
}

oboe::AudioStreamBuilder* BeatriceAudioEngine::setupPlaybackStreamParameters(
    oboe::AudioStreamBuilder* builder) {
  builder->setDataCallback(this)
      ->setErrorCallback(this)
      ->setDeviceId(mPlaybackDeviceId)
      ->setDirection(oboe::Direction::Output)
      ->setChannelCount(mOutputChannelCount);

  return setupCommonStreamParameters(builder);
}

oboe::AudioStreamBuilder* BeatriceAudioEngine::setupCommonStreamParameters(
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
  builder->setUsage(oboe::Usage::Game);
  return builder;
}

void BeatriceAudioEngine::closeStream(
    std::shared_ptr<oboe::AudioStream>& stream) {
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
}

void BeatriceAudioEngine::warnIfNotLowLatency(
    std::shared_ptr<oboe::AudioStream>& stream) {
  if (stream &&
      stream->getPerformanceMode() != oboe::PerformanceMode::LowLatency) {
    LOGW(
        "Stream is NOT low latency."
        "Check your requested format, sample rate and channel count");
  }
}

oboe::DataCallbackResult BeatriceAudioEngine::onAudioReady(
    oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames) {
  if (!mDuplexStream) {
    return oboe::DataCallbackResult::Stop;
  }
  return mDuplexStream->onAudioReady(oboeStream, audioData, numFrames);
}

void BeatriceAudioEngine::onErrorBeforeClose(oboe::AudioStream* oboeStream,
                                             oboe::Result error) {
  LOGE("%s stream Error before close: %s",
       oboe::convertToText(oboeStream->getDirection()),
       oboe::convertToText(error));
}

void BeatriceAudioEngine::onErrorAfterClose(oboe::AudioStream* oboeStream,
                                            oboe::Result error) {
  LOGE("%s stream Error after close: %s",
       oboe::convertToText(oboeStream->getDirection()),
       oboe::convertToText(error));

  closeStreams();

  if (error == oboe::Result::ErrorDisconnected && mProcessorFactory) {
    LOGI("Restarting AudioStream");
    openStreams(mProcessorFactory);
  }
}

int32_t BeatriceAudioEngine::getSampleRate() const { return mSampleRate; }

int32_t BeatriceAudioEngine::getFramesPerBurst() const {
  if (mPlayStream) {
    return mPlayStream->getFramesPerBurst();
  }
  return 0;
}