#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_activity.h>
#include <jni.h>
#include <logging_macros.h>

#include <codecvt>
#include <fstream>
#include <locale>
#include <memory>
#include <string>

#include "beatriceEngine.h"

static const int kOboeApiAAudio = 0;
static const int kOboeApiOpenSLES = 1;

static std::unique_ptr<beatriceEngine> engine = nullptr;

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

JNIEXPORT jboolean JNICALL Java_com_gokrack_beatriceapp_beatriceEngine_create(
    JNIEnv* env, jclass, jobject asset_manager_, jobject dir_name_) {
  // ディレクトリ内に含まれる toml ファイルを検索
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
  if (tomlFiles.size() > 0) {
    // toml ファイルが存在する場合、最初に見つかったものを使用する
    toml_path = tomlFiles.at(0);
  } else {
    // ディレクトリ内に toml ファイルが存在しない場合、assets
    // から必要なファイルをコピーする
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
  engine.reset();
  engine = std::make_unique<beatriceEngine>(toml_path);
  env->ReleaseStringUTFChars(static_cast<jstring>(dir_name_), c_dir_name);
  return (engine) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_delete(JNIEnv* env, jclass) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }
  engine->setEffectOn(false);
  engine.reset();
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_readModel(JNIEnv* env, jclass,
                                                      jobject model_path_) {
  auto c_model_path =
      env->GetStringUTFChars(static_cast<jstring>(model_path_), JNI_FALSE);
  auto model_path = std::string(c_model_path);

  engine = std::make_unique<beatriceEngine>(model_path);

  env->ReleaseStringUTFChars(static_cast<jstring>(model_path_), c_model_path);
  return (engine) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getModelName(JNIEnv* env, jclass) {
  jstring model_name = jstring("<<empty>>");
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return model_name;
  }

  auto u8str = engine->getModelName();
  std::u16string u16str =
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
          .from_bytes(reinterpret_cast<const char*>(u8str.c_str()));

  model_name = env->NewString(reinterpret_cast<const jchar*>(u16str.c_str()),
                              static_cast<jsize>(u16str.length()));

  return model_name;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setEffectOn(JNIEnv* env, jclass,
                                                        jboolean isEffectOn) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  return engine->setEffectOn(isEffectOn) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setRecordingDeviceId(
    JNIEnv* env, jclass, jint deviceId) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }
  engine->setRecordingDeviceId(deviceId);
}

JNIEXPORT void JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPlaybackDeviceId(JNIEnv* env,
                                                                jclass,
                                                                jint deviceId) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return;
  }

  engine->setPlaybackDeviceId(deviceId);
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPerformanceMode(
    JNIEnv* env, jclass type, jint performanceMode_) {
  if (!engine) {
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
Java_com_gokrack_beatriceapp_beatriceEngine_setVoiceID(JNIEnv* env, jclass type,
                                                       jint voiceID) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setVoiceID(voiceID);
  return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_getVoiceName(JNIEnv* env,
                                                         jclass type,
                                                         jint voiceID) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return nullptr;
  }
  std::u8string voiceName = engine->getVoiceName(voiceID);
  return env->NewStringUTF(reinterpret_cast<const char*>(voiceName.c_str()));
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setPitchShift(JNIEnv* env,
                                                          jclass type,
                                                          jfloat pitchShift) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setPitchShift(pitchShift);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setFormantShift(
    JNIEnv* env, jclass type, jfloat formantShift) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setFormantShift(formantShift);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setInputGain(JNIEnv* env,
                                                         jclass type,
                                                         jfloat gain) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setInputGain(gain);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_gokrack_beatriceapp_beatriceEngine_setOutputGain(JNIEnv* env,
                                                          jclass type,
                                                          jfloat gain) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine before calling this "
        "method");
    return JNI_FALSE;
  }

  engine->setInputGain(gain);
  return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_gokrack_beatriceapp_beatriceEngine_setAPI(
    JNIEnv* env, jclass type, jint apiType) {
  if (!engine) {
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
Java_com_gokrack_beatriceapp_beatriceEngine_isAAudioRecommended(JNIEnv* env,
                                                                jclass type) {
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  return engine->isAAudioRecommended() ? JNI_TRUE : JNI_FALSE;
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
  if (!engine) {
    LOGE(
        "Engine is null, you must call createEngine "
        "before calling this method");
    return JNI_FALSE;
  }
  engine->setAsyncMode(isAsyncMode);
  return JNI_TRUE;
}

}  // extern "C"
