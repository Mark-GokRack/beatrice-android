/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BEATRICE_FULLDUPLEXPASS_H
#define BEATRICE_FULLDUPLEXPASS_H

#include <android/log.h>
#include <common/processor_core.h>
#include <oboe/LatencyTuner.h>

#include <asio/bind_executor.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <string>
#include <thread>

class BeatriceFullDuplexPass : public oboe::FullDuplexStream {
 public:
  BeatriceFullDuplexPass(
      std::shared_ptr<beatrice::common::ProcessorCoreBase> processorCore,
      std::shared_ptr<oboe::LatencyTuner> latencyTuner,
      bool useAsyncProcessing = false, size_t frame_size = 480,
      size_t buffer_count = 2)
      : oboe::FullDuplexStream(),
        processorCore_(processorCore),
        latencyTuner_(latencyTuner),
        useAsyncProcessing_(useAsyncProcessing),
        frame_size_(frame_size),
        buffer_size_(buffer_count * frame_size),
        inputBuffer_(useAsyncProcessing
                         ? std::make_unique<float[]>(frame_size * buffer_count)
                         : nullptr),
        outputBuffer_(useAsyncProcessing
                          ? std::make_unique<float[]>(frame_size * buffer_count)
                          : nullptr),
        ioContext_(std::make_shared<asio::io_context>()),
        work_(asio::make_work_guard(*ioContext_.get())),
        ioThread_(useAsyncProcessing ? std::make_unique<std::thread>(
                                           [this]() { ioContext_->run(); })
                                     : nullptr) {}

  virtual ~BeatriceFullDuplexPass() {
    if (ioThread_ && ioThread_->joinable()) {
      ioContext_->stop();
      ioThread_->join();
    }
  };

  virtual oboe::DataCallbackResult onBothStreamsReady(const void* inputData,
                                                      int numInputFrames,
                                                      void* outputData,
                                                      int numOutputFrames) {
    // Copy the input samples to the output with a little arbitrary gain
    // change.

    // This code assumes the data format for both streams is Float.
    const float* inputFloats = static_cast<const float*>(inputData);
    float* outputFloats = static_cast<float*>(outputData);

    // It also assumes the channel count for each stream is the same.
    int32_t numInputChannels = getInputStream()->getChannelCount();
    int32_t numOutputChannels = getOutputStream()->getChannelCount();
    int32_t numInputSamples = numInputFrames * numInputChannels;
    int32_t numOutputSamples = numOutputFrames * numOutputChannels;

    // It is possible that there may be fewer input than output samples.
    int32_t samplesToProcess = std::min(numInputSamples, numOutputSamples);

    if (useAsyncProcessing_ && samplesToProcess <= frame_size_) {
      auto next_buffer_index_ = buffer_index_ + samplesToProcess;
      size_t len = samplesToProcess;
      if (next_buffer_index_ > buffer_size_) {
        len = buffer_size_ - buffer_index_;
      }
      // copy internal output buffer to output
      std::memcpy(outputFloats, &outputBuffer_[buffer_index_],
                  sizeof(float) * len);
      // copy input to internal buffer
      std::memcpy(&inputBuffer_[buffer_index_], inputFloats,
                  sizeof(float) * len);
      if (len < samplesToProcess) {
        // wrap around
        size_t remaining = samplesToProcess - len;
        std::memcpy(&outputFloats[len], &outputBuffer_[0],
                    sizeof(float) * remaining);
        std::memcpy(&inputBuffer_[0], &inputFloats[len],
                    sizeof(float) * remaining);
      }

      size_t frame_idx_ = (buffer_index_ / frame_size_);
      if (next_buffer_index_ / frame_size_ != frame_idx_) {
        // process from internal input buffer to internal output buffer
        auto token = [this, frame_idx_]() {
          const float* inputFloats = &inputBuffer_[frame_idx_ * frame_size_];
          float* outputFloats = &outputBuffer_[frame_idx_ * frame_size_];
          int32_t samplesToProcess = frame_size_;
          processorCore_->Process(inputFloats, outputFloats, samplesToProcess);
        };
        asio::post(*ioContext_.get(), token);
      }

      // update buffer index
      if (next_buffer_index_ >= buffer_size_) {
        next_buffer_index_ -= buffer_size_;
      }
      buffer_index_ = next_buffer_index_;
    } else {
      processorCore_->Process(inputFloats, outputFloats, samplesToProcess);
    }
    /*if( error != beatrice::common::ErrorCode::kSuccess){
      return oboe::DataCallbackResult::Stop;
    }*/

    latencyTuner_->tune();

    //__android_log_print(ANDROID_LOG_INFO, "BEATRICE-ANDROID", "Buffer
    // Size: %d",
    //                   getOutputStream()->getBufferSizeInFrames());
    return oboe::DataCallbackResult::Continue;
  }

 private:
  std::shared_ptr<beatrice::common::ProcessorCoreBase> processorCore_;
  std::shared_ptr<oboe::LatencyTuner> latencyTuner_;

  bool useAsyncProcessing_ = false;
  size_t frame_size_ = 480;
  size_t buffer_size_ = 960;
  size_t buffer_index_ = 0;
  std::unique_ptr<float[]> inputBuffer_;
  std::unique_ptr<float[]> outputBuffer_;
  std::shared_ptr<asio::io_context> ioContext_;
  asio::executor_work_guard<asio::io_context::executor_type> work_;
  std::unique_ptr<std::thread> ioThread_;
};
#endif  // BEATRICE_FULLDUPLEXPASS_H
