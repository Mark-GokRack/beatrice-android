#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_activity.h>
#include <jni.h>
#include <logging_macros.h>

#include <array>
#include <codecvt>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <memory>
#include <string>
#include <vector>

#include "beatriceAudioEngine.h"
#include "beatriceProcessor.h"
#include "effectors/Amplifier.hpp"
#include "effectors/AudioEffectorChain.hpp"
#include "effectors/Compressor.hpp"
#include "effectors/Limiter.hpp"
#include "effectors/NoiseGate.hpp"
#include "effectors/ParametricEqualizer.hpp"

static const int kOboeApiAAudio = 0;
static const int kOboeApiOpenSLES = 1;

static std::unique_ptr<BeatriceAudioEngine> audioEngine = nullptr;
static std::shared_ptr<AudioEffectorChain> effectorChain = nullptr;
static std::shared_ptr<BeatriceProcessor> processor = nullptr;
static std::shared_ptr<Amplifier> amplifier = nullptr;
static std::shared_ptr<Compressor> compressor = nullptr;
static std::shared_ptr<Limiter> limiter = nullptr;
static std::shared_ptr<NoiseGate> noiseGate = nullptr;
static std::shared_ptr<ParametricEqualizer> preEqualizer = nullptr;
static std::shared_ptr<ParametricEqualizer> postEqualizer = nullptr;

namespace {
bool isInitialized() {
  return processor != nullptr && audioEngine != nullptr &&
         effectorChain != nullptr && amplifier != nullptr &&
         compressor != nullptr && limiter != nullptr && noiseGate != nullptr &&
         preEqualizer != nullptr && postEqualizer != nullptr;
}

template <typename T>
bool isEffectorAvailable(const std::shared_ptr<T>& effector,
                         const char* effectorName) {
  if (!effector) {
    LOGE("%s is null, you must call createEngine before calling this method",
         effectorName);
    return false;
  }
  return true;
}

template <typename T, typename Getter>
jdouble getEffectorDouble(const std::shared_ptr<T>& effector,
                          const char* effectorName, Getter getter,
                          jdouble fallback = 0.0) {
  if (!isEffectorAvailable(effector, effectorName)) {
    return fallback;
  }
  return static_cast<jdouble>(getter(*effector));
}

template <typename T, typename Getter>
jboolean getEffectorBoolean(const std::shared_ptr<T>& effector,
                            const char* effectorName, Getter getter) {
  if (!isEffectorAvailable(effector, effectorName)) {
    return JNI_FALSE;
  }
  return getter(*effector) ? JNI_TRUE : JNI_FALSE;
}

std::vector<float> toFloatVector(JNIEnv* env, jdoubleArray inputArray) {
  std::vector<float> result;
  if (!inputArray) {
    return result;
  }

  const jsize length = env->GetArrayLength(inputArray);
  result.resize(static_cast<size_t>(length));

  std::vector<jdouble> temp(static_cast<size_t>(length));
  env->GetDoubleArrayRegion(inputArray, 0, length, temp.data());

  for (jsize i = 0; i < length; ++i) {
    result[static_cast<size_t>(i)] = static_cast<float>(temp[i]);
  }

  return result;
}

jdoubleArray toDoubleArray(JNIEnv* env, const std::vector<float>& values) {
  const jsize length = static_cast<jsize>(values.size());
  jdoubleArray outputArray = env->NewDoubleArray(length);
  if (!outputArray) {
    return nullptr;
  }

  std::vector<jdouble> temp(values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    temp[i] = static_cast<jdouble>(values[i]);
  }

  if (length > 0) {
    env->SetDoubleArrayRegion(outputArray, 0, length, temp.data());
  }

  return outputArray;
}

jdoubleArray makeEmptyDoubleArray(JNIEnv* env) {
  return env->NewDoubleArray(0);
}

std::array<float, beatrice::common::kMaxNSpeakers> toSpeakerMorphingWeights(
    JNIEnv* env, jfloatArray inputArray) {
  std::array<float, beatrice::common::kMaxNSpeakers> result{};
  if (!inputArray) {
    return result;
  }

  const jsize length = env->GetArrayLength(inputArray);
  std::vector<jfloat> temp(static_cast<size_t>(length));
  env->GetFloatArrayRegion(inputArray, 0, length, temp.data());

  const jsize count =
      length < static_cast<jsize>(beatrice::common::kMaxNSpeakers)
          ? length
          : static_cast<jsize>(beatrice::common::kMaxNSpeakers);
  for (jsize i = 0; i < count; ++i) {
    result[static_cast<size_t>(i)] = temp[i];
  }

  return result;
}

void resetEffectorChain() {
  if (effectorChain) {
    effectorChain->clearEffectors();

    effectorChain->addEffector(amplifier);
    effectorChain->addEffector(noiseGate);
    effectorChain->addEffector(compressor);
    effectorChain->addEffector(preEqualizer);
    effectorChain->addEffector(processor);
    effectorChain->addEffector(postEqualizer);
    effectorChain->addEffector(limiter);
  }
}

void copy_from_asset(AAssetManager* assetManager, std::string filename_in_asst,
                     std::string filename) {
  AAsset* asset = AAssetManager_open(assetManager, filename_in_asst.c_str(),
                                     AASSET_MODE_UNKNOWN);
  if (!asset) {
    return;
  }
  off_t length = AAsset_getLength(asset);
  char* buffer = new char[length];
  AAsset_read(asset, buffer, length);
  AAsset_close(asset);

  auto ofs = std::ofstream(filename, std::ios::binary | std::ios::trunc);
  if (ofs) {
    ofs.write(buffer, length);
  }
  ofs.close();
  delete[] buffer;
}

}  // namespace

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_gokrack_beatriceapp_beatriceEngine_create(
    JNIEnv* env, jclass, jobject asset_manager_, jobject dir_name_) {
  auto c_dir_name =
      env->GetStringUTFChars(static_cast<jstring>(dir_name_), JNI_FALSE);
  auto dir_name = std::string(c_dir_name);
  std::vector<std::string> tomlFiles;
  try {
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(dir_name)) {
      if (entry.is_regular_file() && entry.path().extension() == ".toml") {
        tomlFiles.push_back(entry.path().string());
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error during directory traversal: " << e.what() << std::endl;
  }

  std::string toml_path;
  if (!tomlFiles.empty()) {
    toml_path = tomlFiles.at(0);
  } else {
    AAssetManager* assetManager = AAssetManager_fromJava(env, asset_manager_);
    auto model_name = std::string("beatrice_paraphernalia_jvs");
    copy_from_asset(assetManager,
                    model_name + "/beatrice_paraphernalia_jvs.toml",
                    dir_name + std::string("/beatrice_paraphernalia_jvs.toml"));
    copy_from_asset(assetManager, model_name + "/phone_extractor.bin",
                    dir_name + std::string("/phone_extractor.bin"));
    copy_from_asset(assetManager, model_name + "/pitch_estimator.bin",
                    dir_name + std::string("/pitch_estimator.bin"));
    copy_from_asset(assetManager, model_name + "/embedding_setter.bin",
                    dir_name + std::string("/embedding_setter.bin"));
    copy_from_asset(assetManager, model_name + "/waveform_generator.bin",
                    dir_name + std::string("/waveform_generator.bin"));
    copy_from_asset(assetManager, model_name + "/speaker_embeddings.bin",
                    dir_name + std::string("/speaker_embeddings.bin"));
    copy_from_asset(assetManager, model_name + "/noimage.png",
                    dir_name + std::string("/noimage.png"));
    toml_path = dir_name + std::string("/beatrice_paraphernalia_jvs.toml");
  }

  try {
    processor = std::make_shared<BeatriceProcessor>(toml_path);
    audioEngine = std::make_unique<BeatriceAudioEngine>();
    effectorChain = std::make_shared<AudioEffectorChain>();
    amplifier = std::make_shared<Amplifier>(0.0f);
    compressor = std::make_shared<Compressor>();
    limiter = std::make_shared<Limiter>();
    noiseGate = std::make_shared<NoiseGate>();
    preEqualizer = std::make_shared<ParametricEqualizer>(48000.0f, 3);
    postEqualizer = std::make_shared<ParametricEqualizer>(48000.0f, 5);
  } catch (const std::exception& e) {
    LOGE("Failed to create engine: %s", e.what());
    processor.reset();
    audioEngine.reset();
    effectorChain.reset();
    amplifier.reset();
    compressor.reset();
    limiter.reset();
    noiseGate.reset();
    preEqualizer.reset();
    postEqualizer.reset();
  }

  resetEffectorChain();

  env->ReleaseStringUTFChars(static_cast<jstring>(dir_name_), c_dir_name);
  return isInitialized() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_delete(JNIEnv* env, jclass) {
  if (!isInitialized()) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }
  audioEngine->closeStreams();
  processor->resetProcessorCore();
  audioEngine.reset();
  processor.reset();
  effectorChain.reset();
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_readModel(JNIEnv* env, jclass,
                                                      jobject model_path_) {
  auto c_model_path =
      env->GetStringUTFChars(static_cast<jstring>(model_path_), JNI_FALSE);
  auto model_path = std::string(c_model_path);

  auto params = processor ? processor->getParameters() : BeatriceParameters{};
  if (audioEngine) {
    audioEngine->closeStreams();
  }
  try {
    auto nextProcessor = std::make_unique<BeatriceProcessor>(model_path);
    nextProcessor->setParameters(params);
    processor = std::move(nextProcessor);

    resetEffectorChain();

  } catch (const std::exception& e) {
    LOGE("Failed to read model: %s", e.what());
    env->ReleaseStringUTFChars(static_cast<jstring>(model_path_), c_model_path);
    return JNI_FALSE;
  }

  env->ReleaseStringUTFChars(static_cast<jstring>(model_path_), c_model_path);
  return isInitialized() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getModelName(JNIEnv* env, jclass) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return env->NewStringUTF("<<empty>>");
  }
  std::u8string modelName = processor->getModelName();
  return env->NewStringUTF(reinterpret_cast<const char*>(modelName.c_str()));
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getModelDescription(JNIEnv* env,
                                                                jclass) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return env->NewStringUTF("<<empty>>");
  }
  std::u8string modelDescription = processor->getModelDescription();
  return env->NewStringUTF(
      reinterpret_cast<const char*>(modelDescription.c_str()));
}

JNIEXPORT jint JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getModelVersion(JNIEnv* env,
                                                            jclass) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return -1;
  }

  return processor->getModelVersion();
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setEffectOn(JNIEnv* env, jclass,
                                                        jboolean isEffectOn) {
  if (!isInitialized()) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  if (isEffectOn) {
    return audioEngine->setEffectOn(true, effectorChain) ? JNI_TRUE : JNI_FALSE;
  } else {
    const bool success = audioEngine->setEffectOn(false);
    processor->resetProcessorCore();
    return success ? JNI_TRUE : JNI_FALSE;
  }
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setRecordingDeviceId(
    JNIEnv* env, jclass, jint deviceId) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }
  audioEngine->setRecordingDeviceId(deviceId);
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPlaybackDeviceId(JNIEnv* env,
                                                                jclass,
                                                                jint deviceId) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }

  audioEngine->setPlaybackDeviceId(deviceId);
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPerformanceMode(
    JNIEnv* env, jclass type, jint performanceMode_) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }
  oboe::PerformanceMode performanceMode;
  switch (performanceMode_) {
    default:
    case 0:
      performanceMode = oboe::PerformanceMode::LowLatency;
      break;
    case 1:
      performanceMode = oboe::PerformanceMode::None;
      break;
    case 2:
      performanceMode = oboe::PerformanceMode::PowerSaving;
      break;
  }
  audioEngine->setPerformanceMode(performanceMode);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setVoiceID(JNIEnv* env, jclass type,
                                                       jint voiceID) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setVoiceID(voiceID);
  return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getVoiceName(JNIEnv* env,
                                                         jclass type,
                                                         jint voiceID) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return nullptr;
  }
  std::u8string voiceName = processor->getVoiceName(voiceID);
  return env->NewStringUTF(reinterpret_cast<const char*>(voiceName.c_str()));
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getVoiceDescription(JNIEnv* env,
                                                                jclass type,
                                                                jint voiceID) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return nullptr;
  }
  std::u8string voiceDescription = processor->getVoiceDescription(voiceID);
  return env->NewStringUTF(
      reinterpret_cast<const char*>(voiceDescription.c_str()));
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getVoicePortraitPath(JNIEnv* env,
                                                                 jclass type,
                                                                 jint voiceID) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return nullptr;
  }
  std::u8string voicePortraitPath = processor->getVoicePortraitPath(voiceID);
  return env->NewStringUTF(
      reinterpret_cast<const char*>(voicePortraitPath.c_str()));
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getVoicePortraitDescription(
    JNIEnv* env, jclass type, jint voiceID) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return nullptr;
  }
  std::u8string voicePortraitDescription =
      processor->getVoicePortraitDescription(voiceID);
  return env->NewStringUTF(
      reinterpret_cast<const char*>(voicePortraitDescription.c_str()));
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPitchShift(JNIEnv* env,
                                                          jclass type,
                                                          jdouble pitchShift) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setPitchShift(pitchShift);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setFormantShift(
    JNIEnv* env, jclass type, jdouble formantShift) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setFormantShift(formantShift);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setInputGain(JNIEnv* env,
                                                         jclass type,
                                                         jdouble gain) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setInputGain(gain);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setOutputGain(JNIEnv* env,
                                                          jclass type,
                                                          jdouble gain) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setOutputGain(gain);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_gokrack_beatriceapp_beatriceEngine_setAPI(
    JNIEnv* env, jclass type, jint apiType) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }

  oboe::AudioApi audioApi;
  switch (apiType) {
    case kOboeApiAAudio:
      audioApi = oboe::AudioApi::AAudio;
      break;
    case kOboeApiOpenSLES:
      audioApi = oboe::AudioApi::OpenSLES;
      break;
    default:
      LOGE("Unknown API selection to setAPI() %d", apiType);
      return JNI_FALSE;
  }

  return audioEngine->setAudioApi(audioApi) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isAAudioRecommended(JNIEnv* env,
                                                                jclass type) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  return audioEngine->isAAudioRecommended() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_native_1setDefaultStreamValues(
    JNIEnv* env, jclass type, jint sampleRate, jint framesPerBurst) {
  oboe::DefaultStreamValues::SampleRate = (int32_t)sampleRate;
  oboe::DefaultStreamValues::FramesPerBurst = (int32_t)framesPerBurst;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setAsyncMode(JNIEnv* env,
                                                         jclass type,
                                                         jboolean isAsyncMode) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  audioEngine->setAsyncMode(isAsyncMode);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setVoiceCommunicationMode(
    JNIEnv* env, jclass type, jboolean isVoiceCommunicationMode) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  audioEngine->setVoiceCommunicationMode(isVoiceCommunicationMode);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setIntonationIntensity(
    JNIEnv* env, jclass type, jdouble intensity) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setIntonationIntensity(intensity);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPitchCorrection(
    JNIEnv* env, jclass type, jdouble correction) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setPitchCorrection(correction);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPitchCorrectionMode(JNIEnv* env,
                                                                   jclass type,
                                                                   jint mode) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setPitchCorrectionMode(mode);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setSourcePitchRange(
    JNIEnv* env, jclass type, jdouble minPitch, jdouble maxPitch) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  processor->setSourcePitchRange(minPitch, maxPitch);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setVQNumNeighbors(
    JNIEnv* env, jclass type, jint numNeighbors) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }
  processor->setVQNumNeighbors(numNeighbors);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setSpeakerMorphingWeights(
    JNIEnv* env, jclass type, jfloatArray weights) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }
  return processor->setSpeakerMorphingWeights(
             toSpeakerMorphingWeights(env, weights))
             ? JNI_TRUE
             : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setProcessorEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(processor, "Processor")) {
    return JNI_FALSE;
  }
  processor->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setNoiseGateEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(noiseGate, "NoiseGate")) {
    return JNI_FALSE;
  }
  noiseGate->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setNoiseGateThreshold(
    JNIEnv* env, jclass type, jdouble threshold) {
  if (!isEffectorAvailable(noiseGate, "NoiseGate")) {
    return JNI_FALSE;
  }
  noiseGate->setThreshold(static_cast<float>(threshold));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setNoiseGateAttack(JNIEnv* env,
                                                               jclass type,
                                                               jdouble attack) {
  if (!isEffectorAvailable(noiseGate, "NoiseGate")) {
    return JNI_FALSE;
  }
  noiseGate->setAttack(static_cast<float>(attack));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setNoiseGateRelease(
    JNIEnv* env, jclass type, jdouble release) {
  if (!isEffectorAvailable(noiseGate, "NoiseGate")) {
    return JNI_FALSE;
  }
  noiseGate->setRelease(static_cast<float>(release));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setNoiseGateRange(JNIEnv* env,
                                                              jclass type,
                                                              jdouble range) {
  if (!isEffectorAvailable(noiseGate, "NoiseGate")) {
    return JNI_FALSE;
  }
  noiseGate->setRange(static_cast<float>(range));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isNoiseGateEnabled(JNIEnv* env,
                                                               jclass type) {
  return getEffectorBoolean(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.isEnabled();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateThreshold(JNIEnv* env,
                                                                  jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getThreshold();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateAttack(JNIEnv* env,
                                                               jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getAttack();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateRelease(JNIEnv* env,
                                                                jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getRelease();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateRange(JNIEnv* env,
                                                              jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getRange();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateDetectorLevel(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getDetectorLevelDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateGainReduction(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getGainReductionDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateInputPeak(JNIEnv* env,
                                                                  jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getInputPeakDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateOutputPeak(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getOutputPeakDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getNoiseGateGateGain(JNIEnv* env,
                                                                 jclass type) {
  return getEffectorDouble(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.getGateGainDb();
  });
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isNoiseGateOpen(JNIEnv* env,
                                                            jclass type) {
  return getEffectorBoolean(noiseGate, "NoiseGate", [](const NoiseGate& gate) {
    return gate.isGateOpen();
  });
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setAmplifierEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(amplifier, "Amplifier")) {
    return JNI_FALSE;
  }
  amplifier->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setAmplifierGain(JNIEnv* env,
                                                             jclass type,
                                                             jdouble gainDb) {
  if (!isEffectorAvailable(amplifier, "Amplifier")) {
    return JNI_FALSE;
  }
  amplifier->setGain(static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isAmplifierEnabled(JNIEnv* env,
                                                               jclass type) {
  return getEffectorBoolean(amplifier, "Amplifier", [](const Amplifier& value) {
    return value.isEnabled();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getAmplifierGain(JNIEnv* env,
                                                             jclass type) {
  return getEffectorDouble(amplifier, "Amplifier", [](const Amplifier& value) {
    return value.getGain();
  });
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setCompressorEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(compressor, "Compressor")) {
    return JNI_FALSE;
  }
  compressor->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setCompressorThreshold(
    JNIEnv* env, jclass type, jdouble threshold) {
  if (!isEffectorAvailable(compressor, "Compressor")) {
    return JNI_FALSE;
  }
  compressor->setThreshold(static_cast<float>(threshold));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setCompressorAttack(
    JNIEnv* env, jclass type, jdouble attack) {
  if (!isEffectorAvailable(compressor, "Compressor")) {
    return JNI_FALSE;
  }
  compressor->setAttack(static_cast<float>(attack));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setCompressorRelease(
    JNIEnv* env, jclass type, jdouble release) {
  if (!isEffectorAvailable(compressor, "Compressor")) {
    return JNI_FALSE;
  }
  compressor->setRelease(static_cast<float>(release));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setCompressorRatio(JNIEnv* env,
                                                               jclass type,
                                                               jdouble ratio) {
  if (!isEffectorAvailable(compressor, "Compressor")) {
    return JNI_FALSE;
  }
  compressor->setRatio(static_cast<float>(ratio));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setCompressorMakeupGain(
    JNIEnv* env, jclass type, jdouble makeupGain) {
  if (!isEffectorAvailable(compressor, "Compressor")) {
    return JNI_FALSE;
  }
  compressor->setMakeupGain(static_cast<float>(makeupGain));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isCompressorEnabled(JNIEnv* env,
                                                                jclass type) {
  return getEffectorBoolean(
      compressor, "Compressor",
      [](const Compressor& value) { return value.isEnabled(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorThreshold(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getThreshold(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorAttack(JNIEnv* env,
                                                                jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getAttack(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorRelease(JNIEnv* env,
                                                                 jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getRelease(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorRatio(JNIEnv* env,
                                                               jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getRatio(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorMakeupGain(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getMakeupGain(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorDetectorLevel(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getDetectorLevelDb(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorGainReduction(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getGainReductionDb(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorInputPeak(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getInputPeakDb(); });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getCompressorOutputPeak(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(
      compressor, "Compressor",
      [](const Compressor& value) { return value.getOutputPeakDb(); });
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsPeaking(
    JNIEnv* env, jclass type, jint bandIndex, jdouble centerFrequency,
    jdouble q, jdouble gainDb) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsPeaking(bandIndex, static_cast<float>(centerFrequency),
                                 static_cast<float>(q),
                                 static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsLowpass(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsLowpass(bandIndex, static_cast<float>(cutoffFrequency),
                                 static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsHighpass(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsHighpass(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsLowShelf(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q, jdouble gainDb) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsLowShelf(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q),
      static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsHighShelf(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q, jdouble gainDb) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsHighShelf(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q),
      static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsNotch(
    JNIEnv* env, jclass type, jint bandIndex, jdouble centerFrequency,
    jdouble q) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsNotch(bandIndex, static_cast<float>(centerFrequency),
                               static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPreEqualizerBandAsAllpass(
    JNIEnv* env, jclass type, jint bandIndex, jdouble centerFrequency,
    jdouble q) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return JNI_FALSE;
  }
  preEqualizer->setBandAsAllpass(bandIndex, static_cast<float>(centerFrequency),
                                 static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jdoubleArray JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getPreEqualizerFrequencyResponse(
    JNIEnv* env, jclass type, jdoubleArray frequencies) {
  if (!isEffectorAvailable(preEqualizer, "PreEqualizer")) {
    return makeEmptyDoubleArray(env);
  }

  const auto frequencyPoints = toFloatVector(env, frequencies);
  const auto response = preEqualizer->computeFrequencyResponse(frequencyPoints);
  return toDoubleArray(env, response);
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsPeaking(
    JNIEnv* env, jclass type, jint bandIndex, jdouble centerFrequency,
    jdouble q, jdouble gainDb) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsPeaking(
      bandIndex, static_cast<float>(centerFrequency), static_cast<float>(q),
      static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsLowpass(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsLowpass(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsHighpass(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsHighpass(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsLowShelf(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q, jdouble gainDb) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsLowShelf(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q),
      static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsHighShelf(
    JNIEnv* env, jclass type, jint bandIndex, jdouble cutoffFrequency,
    jdouble q, jdouble gainDb) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsHighShelf(
      bandIndex, static_cast<float>(cutoffFrequency), static_cast<float>(q),
      static_cast<float>(gainDb));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsNotch(
    JNIEnv* env, jclass type, jint bandIndex, jdouble centerFrequency,
    jdouble q) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsNotch(bandIndex, static_cast<float>(centerFrequency),
                                static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPostEqualizerBandAsAllpass(
    JNIEnv* env, jclass type, jint bandIndex, jdouble centerFrequency,
    jdouble q) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return JNI_FALSE;
  }
  postEqualizer->setBandAsAllpass(
      bandIndex, static_cast<float>(centerFrequency), static_cast<float>(q));
  return JNI_TRUE;
}

JNIEXPORT jdoubleArray JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getPostEqualizerFrequencyResponse(
    JNIEnv* env, jclass type, jdoubleArray frequencies) {
  if (!isEffectorAvailable(postEqualizer, "PostEqualizer")) {
    return makeEmptyDoubleArray(env);
  }

  const auto frequencyPoints = toFloatVector(env, frequencies);
  const auto response =
      postEqualizer->computeFrequencyResponse(frequencyPoints);
  return toDoubleArray(env, response);
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setLimiterEnabled(
    JNIEnv* env, jclass type, jboolean enabled) {
  if (!isEffectorAvailable(limiter, "Limiter")) {
    return JNI_FALSE;
  }
  limiter->setEnabled(enabled == JNI_TRUE);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setLimiterThreshold(
    JNIEnv* env, jclass type, jdouble threshold) {
  if (!isEffectorAvailable(limiter, "Limiter")) {
    return JNI_FALSE;
  }
  limiter->setThreshold(static_cast<float>(threshold));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setLimiterAttack(JNIEnv* env,
                                                             jclass type,
                                                             jdouble attack) {
  if (!isEffectorAvailable(limiter, "Limiter")) {
    return JNI_FALSE;
  }
  limiter->setAttack(static_cast<float>(attack));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setLimiterRelease(JNIEnv* env,
                                                              jclass type,
                                                              jdouble release) {
  if (!isEffectorAvailable(limiter, "Limiter")) {
    return JNI_FALSE;
  }
  limiter->setRelease(static_cast<float>(release));
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isLimiterEnabled(JNIEnv* env,
                                                             jclass type) {
  return getEffectorBoolean(limiter, "Limiter", [](const Limiter& value) {
    return value.isEnabled();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterThreshold(JNIEnv* env,
                                                                jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getThreshold();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterAttack(JNIEnv* env,
                                                             jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getAttack();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterRelease(JNIEnv* env,
                                                              jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getRelease();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterDetectorLevel(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getDetectorLevelDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterGainReduction(
    JNIEnv* env, jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getGainReductionDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterInputPeak(JNIEnv* env,
                                                                jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getInputPeakDb();
  });
}

JNIEXPORT jdouble JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getLimiterOutputPeak(JNIEnv* env,
                                                                 jclass type) {
  return getEffectorDouble(limiter, "Limiter", [](const Limiter& value) {
    return value.getOutputPeakDb();
  });
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_isLimiterHardClipActive(
    JNIEnv* env, jclass type) {
  return getEffectorBoolean(limiter, "Limiter", [](const Limiter& value) {
    return value.isHardClipActive();
  });
}

JNIEXPORT jint JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getSampleRate(JNIEnv* env,
                                                          jclass type) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return 0;
  }
  return audioEngine->getSampleRate();
}

JNIEXPORT jint JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getFramesPerBurst(JNIEnv* env,
                                                              jclass type) {
  if (!audioEngine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return 0;
  }
  return audioEngine->getFramesPerBurst();
}

}  // extern "C"
