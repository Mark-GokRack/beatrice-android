package com.gokrack.beatriceapp

import android.content.Context
import android.media.AudioManager
import android.os.Build
import android.content.res.AssetManager
object beatriceEngine {

    init {
        System.loadLibrary("beatriceapp")
    }

    // Native methods
    external fun create(assetManager: AssetManager, filesDir: String): Boolean
    external fun isAAudioRecommended(): Boolean
    external fun setAPI(apiType: Int): Boolean
    external fun setEffectOn(isEffectOn: Boolean): Boolean
    external fun setRecordingDeviceId(deviceId: Int)
    external fun setPlaybackDeviceId(deviceId: Int)
    external fun setPerformanceMode(performanceMode: Int): Boolean
    external fun setAsyncMode(isAsyncMode: Boolean): Boolean
    external fun readModel( modelPath : String ):Boolean
    external fun getModelName():String
    external fun setVoiceID(voiceID: Int): Boolean
    external fun getVoiceName(voiceID: Int): String
    external fun setPitchShift(pitchShift: Float): Boolean
    external fun setFormantShift(formantShift: Float): Boolean
    external fun setInputGain(dB: Float): Boolean
    external fun setOutputGain(dB: Float): Boolean

    external fun delete()
    external fun native_setDefaultStreamValues(defaultSampleRate: Int, defaultFramesPerBurst: Int)

    fun setDefaultStreamValues(context: Context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            val audioManager = context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
            val sampleRateStr = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
            val defaultSampleRate = sampleRateStr?.toIntOrNull() ?: return
            val framesPerBurstStr = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER)
            val defaultFramesPerBurst = framesPerBurstStr?.toIntOrNull() ?: return

            native_setDefaultStreamValues(defaultSampleRate, defaultFramesPerBurst)
        }
    }
}