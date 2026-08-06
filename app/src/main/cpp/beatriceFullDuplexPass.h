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
#include <oboe/LatencyTuner.h>

#include <algorithm>
#include <asio/bind_executor.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <cstring>
#include <string>
#include <thread>

#include "effectors/AudioEffector.hpp"

class BeatriceFullDuplexPass : public oboe::FullDuplexStream {
 public:
  BeatriceFullDuplexPass(std::shared_ptr<AudioEffector> effector,
                         std::shared_ptr<oboe::LatencyTuner> latencyTuner,
                         bool useAsyncProcessing = false,
                         size_t buffer_count = 2)
      : oboe::FullDuplexStream(),
        effector_(effector),
        latencyTuner_(latencyTuner),
        useAsyncProcessing_(useAsyncProcessing),
        buffer_size_(buffer_count * frame_size_),
        inputBuffer_(std::make_unique<float[]>(frame_size_ * buffer_count)),
        outputBuffer_(std::make_unique<float[]>(frame_size_ * buffer_count)),
        ioContext_(std::make_shared<asio::io_context>()),
        work_(asio::make_work_guard(*ioContext_.get())),
        ioThread_(useAsyncProcessing ? std::make_unique<std::thread>(
                                           [this]() { ioContext_->run(); })
                                     : nullptr) {
    std::fill_n(inputBuffer_.get(), buffer_size_, 0.0f);
    std::fill_n(outputBuffer_.get(), buffer_size_, 0.0f);
  }

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

    // It also assumes the channel count for each stream is monaural.
    int32_t numInputChannels = getInputStream()->getChannelCount();
    int32_t numOutputChannels = getOutputStream()->getChannelCount();
    if (numInputChannels != 1 || numOutputChannels != 1) {
      __android_log_print(ANDROID_LOG_ERROR, "BeatriceFullDuplexPass",
                          "Channel count must be mono");
      return oboe::DataCallbackResult::Stop;
    }
    int32_t numInputSamples = numInputFrames;
    int32_t numOutputSamples = numOutputFrames;

    // It is possible that there may be fewer input than output samples.
    size_t samplesToProcess = std::min(numInputSamples, numOutputSamples);
    size_t samplesToProcessRemaining = samplesToProcess;
    size_t processedSamples = 0;
    bool async_flag = useAsyncProcessing_ && samplesToProcess <= frame_size_;

    while (samplesToProcessRemaining > 0) {
      size_t len = std::min(frame_size_, samplesToProcessRemaining);
      size_t next_buffer_index_ = buffer_index_ + len;
      if (next_buffer_index_ <= buffer_size_) {
        // copy internal output buffer to output
        std::memcpy(&outputFloats[processedSamples],
                    &outputBuffer_[buffer_index_], sizeof(float) * len);
        // copy input to internal buffer
        std::memcpy(&inputBuffer_[buffer_index_],
                    &inputFloats[processedSamples], sizeof(float) * len);
      } else {
        size_t first_part = buffer_size_ - buffer_index_;
        std::memcpy(&outputFloats[processedSamples],
                    &outputBuffer_[buffer_index_], sizeof(float) * first_part);
        std::memcpy(&inputBuffer_[buffer_index_],
                    &inputFloats[processedSamples], sizeof(float) * first_part);
        size_t second_part = len - first_part;
        std::memcpy(&outputFloats[processedSamples + first_part],
                    &outputBuffer_[0], sizeof(float) * second_part);
        std::memcpy(&inputBuffer_[0],
                    &inputFloats[processedSamples + first_part],
                    sizeof(float) * second_part);
      }

      size_t frame_idx_ = (buffer_index_ / frame_size_);
      size_t next_frame_idx_ = (next_buffer_index_ / frame_size_);
      if (next_frame_idx_ >= (buffer_size_ / frame_size_)) {
        next_frame_idx_ = 0;
      }
      if (effector_ && next_frame_idx_ != frame_idx_) {
        // process from internal input buffer to internal output buffer
        auto func = [this, frame_idx_]() {
          const float* inputFloats = &inputBuffer_[frame_idx_ * frame_size_];
          float* outputFloats = &outputBuffer_[frame_idx_ * frame_size_];
          effector_->process(inputFloats, outputFloats, frame_size_);
        };
        if (async_flag) {
          asio::post(*ioContext_.get(), func);
        } else {
          func();
        }
      }

      // update buffer index
      if (next_buffer_index_ >= buffer_size_) {
        next_buffer_index_ -= buffer_size_;
      }
      buffer_index_ = next_buffer_index_;
      processedSamples += len;
      samplesToProcessRemaining -= len;
    }

    if (latencyTuner_ && !async_flag) {
      latencyTuner_->tune();
    }

    return oboe::DataCallbackResult::Continue;
  }

 private:
  std::shared_ptr<AudioEffector> effector_;
  std::shared_ptr<oboe::LatencyTuner> latencyTuner_;

  bool useAsyncProcessing_ = false;
  static constexpr size_t frame_size_ = 480;
  size_t buffer_size_ = 960;
  size_t buffer_index_ = 0;
  std::unique_ptr<float[]> inputBuffer_;
  std::unique_ptr<float[]> outputBuffer_;
  std::shared_ptr<asio::io_context> ioContext_;
  asio::executor_work_guard<asio::io_context::executor_type> work_;
  std::unique_ptr<std::thread> ioThread_;
};
#endif  // BEATRICE_FULLDUPLEXPASS_H
