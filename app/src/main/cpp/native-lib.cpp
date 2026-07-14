#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_activity.h>
#include <jni.h>
#include <logging_macros.h>

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
#include "effectors/AudioEffectorChain.hpp"

static const int kOboeApiAAudio = 0;
static const int kOboeApiOpenSLES = 1;

static std::shared_ptr<BeatriceProcessor> processor = nullptr;
static std::unique_ptr<BeatriceAudioEngine> audioEngine = nullptr;
static std::shared_ptr<AudioEffectorChain> effectorChain = nullptr;

namespace {

bool isInitialized() {
  return processor != nullptr && audioEngine != nullptr &&
         effectorChain != nullptr;
}

void resetEffectorChain() {
  if (effectorChain) {
    effectorChain->clearEffectors();
    effectorChain->addEffector(processor);
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
  } catch (const std::exception& e) {
    LOGE("Failed to create engine: %s", e.what());
    processor.reset();
    audioEngine.reset();
    effectorChain.reset();
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
  jstring model_name = env->NewStringUTF("<<empty>>");
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return model_name;
  }

  auto u8str = processor->getModelName();
  std::u16string u16str =
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
          .from_bytes(reinterpret_cast<const char*>(u8str.c_str()));

  model_name = env->NewString(reinterpret_cast<const jchar*>(u16str.c_str()),
                              static_cast<jsize>(u16str.length()));

  return model_name;
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getModelDescription(JNIEnv* env,
                                                                jclass) {
  jstring model_description = env->NewStringUTF("<<empty>>");
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return model_description;
  }

  auto u8str = processor->getModelDescription();
  std::u16string u16str =
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
          .from_bytes(reinterpret_cast<const char*>(u8str.c_str()));

  model_description =
      env->NewString(reinterpret_cast<const jchar*>(u16str.c_str()),
                     static_cast<jsize>(u16str.length()));

  return model_description;
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
Java_com_gokrack_beatriceapp_beatriceEngine_setSpeakerMorphingWeight(
    JNIEnv* env, jclass type, jint target_spk, jdouble weight) {
  if (!processor) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }
  processor->setSpeakerMorphingWeight(target_spk, weight);
  return JNI_TRUE;
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
