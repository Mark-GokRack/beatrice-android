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

#include <common/processor_core.h>
#include <android/log.h>
#include <string>

class BeatriceFullDuplexPass : public oboe::FullDuplexStream {
 public:
  BeatriceFullDuplexPass(
      std::shared_ptr<beatrice::common::ProcessorCoreBase> processorCore)
      : oboe::FullDuplexStream(), processorCore_(processorCore) {}
  virtual ~BeatriceFullDuplexPass() = default;

  virtual oboe::DataCallbackResult onBothStreamsReady(const void* inputData,
                                                      int numInputFrames,
                                                      void* outputData,
                                                      int numOutputFrames) {
    // Copy the input samples to the output with a little arbitrary gain change.

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


    //__android_log_write(ANDROID_LOG_INFO, "BEATRICE-ANDROID_input", std::to_string(numInputSamples).c_str());
    //__android_log_write(ANDROID_LOG_INFO, "BEATRICE-ANDROID_output", std::to_string(numOutputSamples).c_str());
    /*  for (int32_t i = 0; i < samplesToProcess; i++) {
          *outputFloats++ = *inputFloats++;  // silence
      }*/
    auto error = processorCore_->Process(inputFloats, outputFloats, samplesToProcess);

    /*if( error != beatrice::common::ErrorCode::kSuccess){
      return oboe::DataCallbackResult::Stop;
    }*/
    return oboe::DataCallbackResult::Continue;
  }

 private:
  std::shared_ptr<beatrice::common::ProcessorCoreBase> processorCore_;
};
#endif  // BEATRICE_FULLDUPLEXPASS_H
