#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_activity.h>
#include <jni.h>
#include <logging_macros.h>

#include <fstream>
#include <string>

#include "beatriceEngine.h"

static const int kOboeApiAAudio = 0;
static const int kOboeApiOpenSLES = 1;

static beatriceEngine* engine = nullptr;

void copy_from_asset(AAssetManager* assetManager, std::string filename_in_asst,
                     std::string filename) {
  {
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
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_create(JNIEnv* env, jclass,
                                                       jobject asset_manager_,
                                                       jobject dir_name_) {
  if (engine == nullptr) {
    AAssetManager* assetManager = AAssetManager_fromJava(env, asset_manager_);
    auto c_dir_name =
        env->GetStringUTFChars(static_cast<jstring>(dir_name_), JNI_FALSE);
    auto dir_name = std::string(c_dir_name);
    // auto model_name = std::string("beatrice_paraphernalia_jvs");
    auto model_name = std::string("gokrack");
    copy_from_asset(assetManager,
                    model_name + "/beatrice_paraphernalia_jvs.toml",
                    dir_name + std::string("/beatrice_paraphernalia_jvs.toml"));
    // copy_from_asset(assetManager, "beatrice_paraphernalia_jvs/noimage.png",
    //                 dir_name + std::string("/noimage.png"));
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

    engine = new beatriceEngine(
        dir_name + std::string("/beatrice_paraphernalia_jvs.toml"));

    env->ReleaseStringUTFChars(static_cast<jstring>(dir_name_), c_dir_name);
  }
  return (engine != nullptr) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_delete(JNIEnv* env, jclass) {
  if (engine) {
    engine->setEffectOn(false);
    delete engine;
    engine = nullptr;
  }
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setEffectOn(
    JNIEnv* env, jclass, jboolean isEffectOn) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  return engine->setEffectOn(isEffectOn) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setRecordingDeviceId(
    JNIEnv* env, jclass, jint deviceId) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }
  engine->setRecordingDeviceId(deviceId);
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setPlaybackDeviceId(
    JNIEnv* env, jclass, jint deviceId) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }

  engine->setPlaybackDeviceId(deviceId);
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setPerformanceMode(
    JNIEnv* env, jclass type, jint performanceMode_) {
  if (engine == nullptr) {
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
  engine->setPerformanceMode(performanceMode);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setVoiceID(JNIEnv* env,
                                                           jclass type,
                                                           jint voiceID) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setVoiceID(voiceID);
  return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_getVoiceName(JNIEnv* env,
                                                             jclass type,
                                                             jint voiceID) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return nullptr;
  }
  std::u8string voiceName = engine->getVoiceName(voiceID);
  return env->NewStringUTF(reinterpret_cast<const char*>(voiceName.c_str()));
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setPitchShift(
    JNIEnv* env, jclass type, jfloat pitchShift) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setPitchShift(pitchShift);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setFormantShift(
    JNIEnv* env, jclass type, jfloat formantShift) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setFormantShift(formantShift);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setInputGain(JNIEnv* env,
                                                             jclass type,
                                                             jfloat gain) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setInputGain(gain);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setOutputGain(JNIEnv* env,
                                                              jclass type,
                                                              jfloat gain) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setInputGain(gain);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setAPI(JNIEnv* env, jclass type,
                                                       jint apiType) {
  if (engine == nullptr) {
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

  return engine->setAudioApi(audioApi) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_isAAudioRecommended(
    JNIEnv* env, jclass type) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  return engine->isAAudioRecommended() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_native_1setDefaultStreamValues(
    JNIEnv* env, jclass type, jint sampleRate, jint framesPerBurst) {
  oboe::DefaultStreamValues::SampleRate = (int32_t)sampleRate;
  oboe::DefaultStreamValues::FramesPerBurst = 480;  //(int32_t)framesPerBurst;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceAndroid_beatriceEngine_setAsyncMode(
    JNIEnv* env, jclass type, jboolean isAsyncMode) {
  if (engine == nullptr) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  engine->setAsyncMode(isAsyncMode);
  return JNI_TRUE;
}

}  // extern "C"
