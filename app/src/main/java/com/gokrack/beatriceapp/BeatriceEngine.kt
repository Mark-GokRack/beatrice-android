package com.gokrack.beatriceapp

import android.content.Context
import android.media.AudioManager
import android.os.Build
import android.content.res.AssetManager
import kotlin.math.exp
import kotlin.math.ln

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
    external fun isNoiseGateEnabled(): Boolean
    external fun getNoiseGateThreshold(): Double
    external fun getNoiseGateAttack(): Double
    external fun getNoiseGateRelease(): Double
    external fun getNoiseGateRange(): Double
    external fun getNoiseGateDetectorLevel(): Double
    external fun getNoiseGateGainReduction(): Double
    external fun getNoiseGateInputPeak(): Double
    external fun getNoiseGateOutputPeak(): Double
    external fun getNoiseGateGateGain(): Double
    external fun isNoiseGateOpen(): Boolean
    external fun setAmplifierEnabled(enabled: Boolean): Boolean
    external fun setAmplifierGain(gainDb: Double): Boolean
    external fun isAmplifierEnabled(): Boolean
    external fun getAmplifierGain(): Double
    external fun setCompressorEnabled(enabled: Boolean): Boolean
    external fun setCompressorThreshold(threshold: Double): Boolean
    external fun setCompressorAttack(attack: Double): Boolean
    external fun setCompressorRelease(release: Double): Boolean
    external fun setCompressorRatio(ratio: Double): Boolean
    external fun setCompressorMakeupGain(makeupGain: Double): Boolean
    external fun isCompressorEnabled(): Boolean
    external fun getCompressorThreshold(): Double
    external fun getCompressorAttack(): Double
    external fun getCompressorRelease(): Double
    external fun getCompressorRatio(): Double
    external fun getCompressorMakeupGain(): Double
    external fun getCompressorDetectorLevel(): Double
    external fun getCompressorGainReduction(): Double
    external fun getCompressorInputPeak(): Double
    external fun getCompressorOutputPeak(): Double
    external fun setPreEqualizerEnabled(enabled: Boolean): Boolean
    external fun setPreEqualizerBandAsPeaking(bandIndex: Int, centerFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPreEqualizerBandAsLowpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPreEqualizerBandAsHighpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPreEqualizerBandAsLowShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPreEqualizerBandAsHighShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPreEqualizerBandAsNotch(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun setPreEqualizerBandAsAllpass(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun getPreEqualizerFrequencyResponse(frequencies: DoubleArray): DoubleArray
    external fun setPostEqualizerEnabled(enabled: Boolean): Boolean
    external fun setPostEqualizerBandAsPeaking(bandIndex: Int, centerFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPostEqualizerBandAsLowpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerBandAsHighpass(bandIndex: Int, cutoffFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerBandAsLowShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPostEqualizerBandAsHighShelf(bandIndex: Int, cutoffFrequency: Double, q: Double, gainDb: Double): Boolean
    external fun setPostEqualizerBandAsNotch(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun setPostEqualizerBandAsAllpass(bandIndex: Int, centerFrequency: Double, q: Double): Boolean
    external fun getPostEqualizerFrequencyResponse(frequencies: DoubleArray): DoubleArray
    external fun setLimiterEnabled(enabled: Boolean): Boolean
    external fun setLimiterThreshold(threshold: Double): Boolean
    external fun setLimiterAttack(attack: Double): Boolean
    external fun setLimiterRelease(release: Double): Boolean
    external fun isLimiterEnabled(): Boolean
    external fun getLimiterThreshold(): Double
    external fun getLimiterAttack(): Double
    external fun getLimiterRelease(): Double
    external fun getLimiterDetectorLevel(): Double
    external fun getLimiterGainReduction(): Double
    external fun getLimiterInputPeak(): Double
    external fun getLimiterOutputPeak(): Double
    external fun isLimiterHardClipActive(): Boolean

    external fun delete()
    external fun native_setDefaultStreamValues(defaultSampleRate: Int, defaultFramesPerBurst: Int)
    external fun getSampleRate(): Int
    external fun getFramesPerBurst(): Int

    fun generateLogFrequencies(
        pointCount: Int = 256,
        minHz: Double = 20.0,
        maxHz: Double = 20000.0
    ): DoubleArray {
        require(pointCount >= 2) { "pointCount must be >= 2" }
        require(minHz > 0.0) { "minHz must be > 0" }
        require(maxHz > minHz) { "maxHz must be greater than minHz" }

        val result = DoubleArray(pointCount)
        val minLog = ln(minHz)
        val maxLog = ln(maxHz)
        val step = (maxLog - minLog) / (pointCount - 1)

        for (i in 0 until pointCount) {
            result[i] = exp(minLog + step * i)
        }

        return result
    }

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