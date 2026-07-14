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
    external fun getModelDescription():String
    external fun getModelVersion():Int
    external fun setVoiceID(voiceID: Int): Boolean
    external fun getVoiceName(voiceID: Int): String
    external fun getVoiceDescription(voiceID: Int): String
    external fun getVoicePortraitPath(voiceID: Int): String
    external fun getVoicePortraitDescription(voiceID: Int): String
    external fun setPitchShift(pitchShift: Double): Boolean
    external fun setFormantShift(formantShift: Double): Boolean
    external fun setInputGain(dB: Double): Boolean
    external fun setOutputGain(dB: Double): Boolean
    external fun setIntonationIntensity(intensity: Double): Boolean
    external fun setPitchCorrection(correction: Double): Boolean
    external fun setPitchCorrectionMode(mode: Int): Boolean
    external fun setSourcePitchRange(minPitch: Double, maxPitch: Double): Boolean
    external fun setVQNumNeighbors(numNeighbors: Int): Boolean
    external fun setSpeakerMorphingWeight(targetSpk: Int, weight: Double): Boolean
    external fun setProcessorEnabled(enabled: Boolean): Boolean
    external fun setNoiseGateEnabled(enabled: Boolean): Boolean
    external fun setNoiseGateThreshold(threshold: Double): Boolean
    external fun setNoiseGateAttack(attack: Double): Boolean
    external fun setNoiseGateRelease(release: Double): Boolean
    external fun setNoiseGateRange(range: Double): Boolean
    external fun setAmplifierEnabled(enabled: Boolean): Boolean
    external fun setAmplifierGain(gainDb: Double): Boolean
    external fun setCompressorEnabled(enabled: Boolean): Boolean
    external fun setCompressorThreshold(threshold: Double): Boolean
    external fun setCompressorAttack(attack: Double): Boolean
    external fun setCompressorRelease(release: Double): Boolean
    external fun setCompressorRatio(ratio: Double): Boolean
    external fun setCompressorMakeupGain(makeupGain: Double): Boolean
    external fun setPreEqualizerEnabled(enabled: Boolean): Boolean
    external fun setPreEqualizerBandAsPeaking(bandIndex: Int, centerFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPreEqualizerBandAsLowpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPreEqualizerBandAsHighpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPreEqualizerBandAsLowShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPreEqualizerBandAsHighShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPreEqualizerBandAsNotch(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun setPreEqualizerBandAsAllpass(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerEnabled(enabled: Boolean): Boolean
    external fun setPostEqualizerBandAsPeaking(bandIndex: Int, centerFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPostEqualizerBandAsLowpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerBandAsHighpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerBandAsLowShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPostEqualizerBandAsHighShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPostEqualizerBandAsNotch(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerBandAsAllpass(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun setLimiterEnabled(enabled: Boolean): Boolean
    external fun setLimiterThreshold(threshold: Double): Boolean
    external fun setLimiterAttack(attack: Double): Boolean
    external fun setLimiterRelease(release: Double): Boolean

    external fun delete()
    external fun native_setDefaultStreamValues(defaultSampleRate: Int, defaultFramesPerBurst: Int)
    external fun getSampleRate(): Int
    external fun getFramesPerBurst(): Int

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