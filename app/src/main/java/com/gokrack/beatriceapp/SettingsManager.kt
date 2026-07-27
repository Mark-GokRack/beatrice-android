package com.gokrack.beatriceapp

import android.content.Context
import android.content.SharedPreferences
import java.util.Locale

/**
 * Manages persistent storage for voice, params, and morphing settings.
 */
object SettingsManager {
    private const val PREFS_NAME = "beatrice_settings"
    private const val KEY_INPUT_GAIN = "input_gain"
    private const val KEY_OUTPUT_GAIN = "output_gain"
    private const val KEY_PITCH_SHIFT = "pitch_shift"
    private const val KEY_FORMANT_SHIFT = "formant_shift"
    private const val KEY_VQ_NEIGHBORS = "vq_neighbors"
    private const val KEY_INTONATION_INTENSITY = "intonation_intensity"
    private const val KEY_PITCH_CORRECTION = "pitch_correction"
    private const val KEY_PITCH_CORRECTION_MODE = "pitch_correction_mode"
    private const val KEY_SOURCE_PITCH_RANGE_MIN = "source_pitch_range_min"
    private const val KEY_SOURCE_PITCH_RANGE_MAX = "source_pitch_range_max"
    private const val KEY_MORPHING_WEIGHTS = "morphing_weights"
    private const val KEY_VOICE_ID = "voice_id"

    // Effector settings
    private const val KEY_AMPLIFIER_ENABLED = "amplifier_enabled"
    private const val KEY_AMPLIFIER_GAIN = "amplifier_gain"
    private const val KEY_NOISE_GATE_ENABLED = "noise_gate_enabled"
    private const val KEY_NOISE_GATE_THRESHOLD = "noise_gate_threshold"
    private const val KEY_NOISE_GATE_RANGE = "noise_gate_range"
    private const val KEY_NOISE_GATE_ATTACK = "noise_gate_attack"
    private const val KEY_NOISE_GATE_RELEASE = "noise_gate_release"
    private const val KEY_COMPRESSOR_ENABLED = "compressor_enabled"
    private const val KEY_COMPRESSOR_THRESHOLD = "compressor_threshold"
    private const val KEY_COMPRESSOR_RATIO = "compressor_ratio"
    private const val KEY_COMPRESSOR_ATTACK = "compressor_attack"
    private const val KEY_COMPRESSOR_RELEASE = "compressor_release"
    private const val KEY_COMPRESSOR_MAKEUP_GAIN = "compressor_makeup_gain"
    private const val KEY_LIMITER_ENABLED = "limiter_enabled"
    private const val KEY_LIMITER_THRESHOLD = "limiter_threshold"
    private const val KEY_LIMITER_ATTACK = "limiter_attack"
    private const val KEY_LIMITER_RELEASE = "limiter_release"
    private const val KEY_PRE_EQUALIZER_ENABLED = "pre_equalizer_enabled"
    private const val KEY_POST_EQUALIZER_ENABLED = "post_equalizer_enabled"

    const val DEFAULT_INPUT_GAIN = 0.0f
    const val DEFAULT_OUTPUT_GAIN = 0.0f
    const val DEFAULT_PITCH_SHIFT = 0.0f
    const val DEFAULT_FORMANT_SHIFT = 0.0f
    const val DEFAULT_VQ_NEIGHBORS = 1
    const val DEFAULT_INTONATION_INTENSITY = 1.0f
    const val DEFAULT_PITCH_CORRECTION = 0.0f
    const val DEFAULT_PITCH_CORRECTION_MODE = 0
    const val DEFAULT_SOURCE_PITCH_MIN = 33.125f
    const val DEFAULT_SOURCE_PITCH_MAX = 80.875f
    const val DEFAULT_VOICE_ID = 0

    // Effector defaults
    const val DEFAULT_AMPLIFIER_ENABLED = true
    const val DEFAULT_AMPLIFIER_GAIN = 0.0f
    const val DEFAULT_NOISE_GATE_ENABLED = false
    const val DEFAULT_NOISE_GATE_THRESHOLD = -40.0f
    const val DEFAULT_NOISE_GATE_RANGE = -80.0f
    const val DEFAULT_NOISE_GATE_ATTACK = 5.0f
    const val DEFAULT_NOISE_GATE_RELEASE = 50.0f
    const val DEFAULT_COMPRESSOR_ENABLED = false
    const val DEFAULT_COMPRESSOR_THRESHOLD = -12.0f
    const val DEFAULT_COMPRESSOR_RATIO = 2.0f
    const val DEFAULT_COMPRESSOR_ATTACK = 5.0f
    const val DEFAULT_COMPRESSOR_RELEASE = 50.0f
    const val DEFAULT_COMPRESSOR_MAKEUP_GAIN = 0.0f
    const val DEFAULT_LIMITER_ENABLED = true
    const val DEFAULT_LIMITER_THRESHOLD = -3.0f
    const val DEFAULT_LIMITER_ATTACK = 5.0f
    const val DEFAULT_LIMITER_RELEASE = 50.0f
    const val DEFAULT_PRE_EQUALIZER_ENABLED = false
    const val DEFAULT_POST_EQUALIZER_ENABLED = false
    const val DEFAULT_EQ_GAIN = 0.0f
    const val MIN_EQ_GAIN = -30.0f
    const val MAX_EQ_GAIN = 30.0f
    const val DEFAULT_EQ_Q = 0.7f
    val DEFAULT_PRE_EQ_FREQUENCIES = floatArrayOf(100f, 1000f, 10000f)
    val DEFAULT_POST_EQ_FREQUENCIES = floatArrayOf(30f, 100f, 300f, 1000f, 10000f)
    const val DEFAULT_EQ_TYPE = 0 // Peaking

    private val DEFAULT_MORPHING_WEIGHTS = FloatArray(256) { 0.0f }.apply{ this[0] = 1.0f } // Default to first voice

    private lateinit var prefs: SharedPreferences

    fun init(context: Context) {
        prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    }

    fun saveInputGain(value: Float) {
        prefs.edit().putFloat(KEY_INPUT_GAIN, value).apply()
    }

    fun loadInputGain(): Float = prefs.getFloat(KEY_INPUT_GAIN, DEFAULT_INPUT_GAIN)

    fun saveOutputGain(value: Float) {
        prefs.edit().putFloat(KEY_OUTPUT_GAIN, value).apply()
    }

    fun loadOutputGain(): Float = prefs.getFloat(KEY_OUTPUT_GAIN, DEFAULT_OUTPUT_GAIN)

    fun savePitchShift(value: Float) {
        prefs.edit().putFloat(KEY_PITCH_SHIFT, value).apply()
    }

    fun loadPitchShift(): Float = prefs.getFloat(KEY_PITCH_SHIFT, DEFAULT_PITCH_SHIFT)

    fun saveFormantShift(value: Float) {
        prefs.edit().putFloat(KEY_FORMANT_SHIFT, value).apply()
    }

    fun loadFormantShift(): Float = prefs.getFloat(KEY_FORMANT_SHIFT, DEFAULT_FORMANT_SHIFT)

    fun saveVQNeighbors(value: Int) {
        prefs.edit().putInt(KEY_VQ_NEIGHBORS, value).apply()
    }

    fun loadVQNeighbors(): Int = prefs.getInt(KEY_VQ_NEIGHBORS, DEFAULT_VQ_NEIGHBORS)

    fun saveIntonationIntensity(value: Float) {
        prefs.edit().putFloat(KEY_INTONATION_INTENSITY, value).apply()
    }

    fun loadIntonationIntensity(): Float = prefs.getFloat(KEY_INTONATION_INTENSITY, DEFAULT_INTONATION_INTENSITY)

    fun savePitchCorrection(value: Float) {
        prefs.edit().putFloat(KEY_PITCH_CORRECTION, value).apply()
    }

    fun loadPitchCorrection(): Float = prefs.getFloat(KEY_PITCH_CORRECTION, DEFAULT_PITCH_CORRECTION)

    fun savePitchCorrectionMode(value: Int) {
        prefs.edit().putInt(KEY_PITCH_CORRECTION_MODE, value).apply()
    }

    fun loadPitchCorrectionMode(): Int = prefs.getInt(KEY_PITCH_CORRECTION_MODE, DEFAULT_PITCH_CORRECTION_MODE)

    fun saveSourcePitchRange(min: Float, max: Float) {
        prefs.edit()
            .putFloat(KEY_SOURCE_PITCH_RANGE_MIN, min)
            .putFloat(KEY_SOURCE_PITCH_RANGE_MAX, max)
            .apply()
    }

    fun loadSourcePitchRangeMin(): Float = prefs.getFloat(KEY_SOURCE_PITCH_RANGE_MIN, DEFAULT_SOURCE_PITCH_MIN)

    fun loadSourcePitchRangeMax(): Float = prefs.getFloat(KEY_SOURCE_PITCH_RANGE_MAX, DEFAULT_SOURCE_PITCH_MAX)

    fun saveVoiceId(value: Int) {
        prefs.edit().putInt(KEY_VOICE_ID, value).apply()
    }

    fun loadVoiceId(): Int = prefs.getInt(KEY_VOICE_ID, DEFAULT_VOICE_ID)

    fun saveMorphingWeights(weights: FloatArray) {
        val serialized = weights.joinToString(";") { String.format(Locale.US, "%.6f", it) }
        prefs.edit().putString(KEY_MORPHING_WEIGHTS, serialized).apply()
    }

    fun resetMorphingWeights() {
        prefs.edit().putString(KEY_MORPHING_WEIGHTS, null).apply()
    }

    fun loadMorphingWeights(): FloatArray {
        val serialized = prefs.getString(KEY_MORPHING_WEIGHTS, null) ?: return DEFAULT_MORPHING_WEIGHTS.copyOf()
        val parsed = serialized.split(";").mapNotNull { it.toFloatOrNull() }
        return if (parsed.size == DEFAULT_MORPHING_WEIGHTS.size) {
            parsed.toFloatArray()
        } else {
            DEFAULT_MORPHING_WEIGHTS.copyOf()
        }
    }

    // Effector helpers
    private fun eqTypeKey(isPre: Boolean, band: Int) = "${if (isPre) "pre" else "post"}_eq_band_${band}_type"
    private fun eqFreqKey(isPre: Boolean, band: Int) = "${if (isPre) "pre" else "post"}_eq_band_${band}_freq"
    private fun eqQKey(isPre: Boolean, band: Int) = "${if (isPre) "pre" else "post"}_eq_band_${band}_q"
    private fun eqGainKey(isPre: Boolean, band: Int) = "${if (isPre) "pre" else "post"}_eq_band_${band}_gain"

    fun saveEqualizerBand(isPre: Boolean, band: Int, type: Int, frequency: Float, q: Float, gain: Float) {
        prefs.edit()
            .putInt(eqTypeKey(isPre, band), type)
            .putFloat(eqFreqKey(isPre, band), frequency)
            .putFloat(eqQKey(isPre, band), q)
            .putFloat(eqGainKey(isPre, band), gain)
            .apply()
    }

    fun loadEqualizerBand(isPre: Boolean, band: Int): EqualizerBandSettings {
        val defaultFreq = if (isPre) {
            DEFAULT_PRE_EQ_FREQUENCIES.getOrElse(band) { 1000f }
        } else {
            DEFAULT_POST_EQ_FREQUENCIES.getOrElse(band) { 1000f }
        }
        return EqualizerBandSettings(
            type = prefs.getInt(eqTypeKey(isPre, band), DEFAULT_EQ_TYPE).coerceIn(0, 6),
            frequency = prefs.getFloat(eqFreqKey(isPre, band), defaultFreq),
            q = prefs.getFloat(eqQKey(isPre, band), DEFAULT_EQ_Q),
            gain = prefs.getFloat(eqGainKey(isPre, band), DEFAULT_EQ_GAIN).coerceIn(MIN_EQ_GAIN, MAX_EQ_GAIN)
        )
    }

    fun saveAmplifierEnabled(enabled: Boolean) = prefs.edit().putBoolean(KEY_AMPLIFIER_ENABLED, enabled).apply()
    fun loadAmplifierEnabled(): Boolean = prefs.getBoolean(KEY_AMPLIFIER_ENABLED, DEFAULT_AMPLIFIER_ENABLED)
    fun saveAmplifierGain(value: Float) = prefs.edit().putFloat(KEY_AMPLIFIER_GAIN, value).apply()
    fun loadAmplifierGain(): Float = prefs.getFloat(KEY_AMPLIFIER_GAIN, DEFAULT_AMPLIFIER_GAIN)

    fun saveNoiseGateEnabled(enabled: Boolean) = prefs.edit().putBoolean(KEY_NOISE_GATE_ENABLED, enabled).apply()
    fun loadNoiseGateEnabled(): Boolean = prefs.getBoolean(KEY_NOISE_GATE_ENABLED, DEFAULT_NOISE_GATE_ENABLED)
    fun saveNoiseGateThreshold(value: Float) = prefs.edit().putFloat(KEY_NOISE_GATE_THRESHOLD, value).apply()
    fun loadNoiseGateThreshold(): Float = prefs.getFloat(KEY_NOISE_GATE_THRESHOLD, DEFAULT_NOISE_GATE_THRESHOLD)
    fun saveNoiseGateRange(value: Float) = prefs.edit().putFloat(KEY_NOISE_GATE_RANGE, value).apply()
    fun loadNoiseGateRange(): Float = prefs.getFloat(KEY_NOISE_GATE_RANGE, DEFAULT_NOISE_GATE_RANGE)
    fun saveNoiseGateAttack(value: Float) = prefs.edit().putFloat(KEY_NOISE_GATE_ATTACK, value).apply()
    fun loadNoiseGateAttack(): Float = prefs.getFloat(KEY_NOISE_GATE_ATTACK, DEFAULT_NOISE_GATE_ATTACK)
    fun saveNoiseGateRelease(value: Float) = prefs.edit().putFloat(KEY_NOISE_GATE_RELEASE, value).apply()
    fun loadNoiseGateRelease(): Float = prefs.getFloat(KEY_NOISE_GATE_RELEASE, DEFAULT_NOISE_GATE_RELEASE)

    fun saveCompressorEnabled(enabled: Boolean) = prefs.edit().putBoolean(KEY_COMPRESSOR_ENABLED, enabled).apply()
    fun loadCompressorEnabled(): Boolean = prefs.getBoolean(KEY_COMPRESSOR_ENABLED, DEFAULT_COMPRESSOR_ENABLED)
    fun saveCompressorThreshold(value: Float) = prefs.edit().putFloat(KEY_COMPRESSOR_THRESHOLD, value).apply()
    fun loadCompressorThreshold(): Float = prefs.getFloat(KEY_COMPRESSOR_THRESHOLD, DEFAULT_COMPRESSOR_THRESHOLD)
    fun saveCompressorRatio(value: Float) = prefs.edit().putFloat(KEY_COMPRESSOR_RATIO, value).apply()
    fun loadCompressorRatio(): Float = prefs.getFloat(KEY_COMPRESSOR_RATIO, DEFAULT_COMPRESSOR_RATIO)
    fun saveCompressorAttack(value: Float) = prefs.edit().putFloat(KEY_COMPRESSOR_ATTACK, value).apply()
    fun loadCompressorAttack(): Float = prefs.getFloat(KEY_COMPRESSOR_ATTACK, DEFAULT_COMPRESSOR_ATTACK)
    fun saveCompressorRelease(value: Float) = prefs.edit().putFloat(KEY_COMPRESSOR_RELEASE, value).apply()
    fun loadCompressorRelease(): Float = prefs.getFloat(KEY_COMPRESSOR_RELEASE, DEFAULT_COMPRESSOR_RELEASE)
    fun saveCompressorMakeupGain(value: Float) = prefs.edit().putFloat(KEY_COMPRESSOR_MAKEUP_GAIN, value).apply()
    fun loadCompressorMakeupGain(): Float = prefs.getFloat(KEY_COMPRESSOR_MAKEUP_GAIN, DEFAULT_COMPRESSOR_MAKEUP_GAIN)

    fun saveLimiterEnabled(enabled: Boolean) = prefs.edit().putBoolean(KEY_LIMITER_ENABLED, enabled).apply()
    fun loadLimiterEnabled(): Boolean = prefs.getBoolean(KEY_LIMITER_ENABLED, DEFAULT_LIMITER_ENABLED)
    fun saveLimiterThreshold(value: Float) = prefs.edit().putFloat(KEY_LIMITER_THRESHOLD, value).apply()
    fun loadLimiterThreshold(): Float = prefs.getFloat(KEY_LIMITER_THRESHOLD, DEFAULT_LIMITER_THRESHOLD)
    fun saveLimiterAttack(value: Float) = prefs.edit().putFloat(KEY_LIMITER_ATTACK, value).apply()
    fun loadLimiterAttack(): Float = prefs.getFloat(KEY_LIMITER_ATTACK, DEFAULT_LIMITER_ATTACK)
    fun saveLimiterRelease(value: Float) = prefs.edit().putFloat(KEY_LIMITER_RELEASE, value).apply()
    fun loadLimiterRelease(): Float = prefs.getFloat(KEY_LIMITER_RELEASE, DEFAULT_LIMITER_RELEASE)

    fun savePreEqualizerEnabled(enabled: Boolean) = prefs.edit().putBoolean(KEY_PRE_EQUALIZER_ENABLED, enabled).apply()
    fun loadPreEqualizerEnabled(): Boolean = prefs.getBoolean(KEY_PRE_EQUALIZER_ENABLED, DEFAULT_PRE_EQUALIZER_ENABLED)
    fun savePostEqualizerEnabled(enabled: Boolean) = prefs.edit().putBoolean(KEY_POST_EQUALIZER_ENABLED, enabled).apply()
    fun loadPostEqualizerEnabled(): Boolean = prefs.getBoolean(KEY_POST_EQUALIZER_ENABLED, DEFAULT_POST_EQUALIZER_ENABLED)

    fun resetAllToDefaults() {
        val editor = prefs.edit().clear()
        editor.putFloat(KEY_INPUT_GAIN, DEFAULT_INPUT_GAIN)
        editor.putFloat(KEY_OUTPUT_GAIN, DEFAULT_OUTPUT_GAIN)
        editor.putFloat(KEY_PITCH_SHIFT, DEFAULT_PITCH_SHIFT)
        editor.putFloat(KEY_FORMANT_SHIFT, DEFAULT_FORMANT_SHIFT)
        editor.putInt(KEY_VQ_NEIGHBORS, DEFAULT_VQ_NEIGHBORS)
        editor.putFloat(KEY_INTONATION_INTENSITY, DEFAULT_INTONATION_INTENSITY)
        editor.putFloat(KEY_PITCH_CORRECTION, DEFAULT_PITCH_CORRECTION)
        editor.putInt(KEY_PITCH_CORRECTION_MODE, DEFAULT_PITCH_CORRECTION_MODE)
        editor.putFloat(KEY_SOURCE_PITCH_RANGE_MIN, DEFAULT_SOURCE_PITCH_MIN)
        editor.putFloat(KEY_SOURCE_PITCH_RANGE_MAX, DEFAULT_SOURCE_PITCH_MAX)
        editor.putInt(KEY_VOICE_ID, DEFAULT_VOICE_ID)
        editor.putString(KEY_MORPHING_WEIGHTS, DEFAULT_MORPHING_WEIGHTS.joinToString(";") { it.toString() })

        editor.putBoolean(KEY_AMPLIFIER_ENABLED, DEFAULT_AMPLIFIER_ENABLED)
        editor.putFloat(KEY_AMPLIFIER_GAIN, DEFAULT_AMPLIFIER_GAIN)
        editor.putBoolean(KEY_NOISE_GATE_ENABLED, DEFAULT_NOISE_GATE_ENABLED)
        editor.putFloat(KEY_NOISE_GATE_THRESHOLD, DEFAULT_NOISE_GATE_THRESHOLD)
        editor.putFloat(KEY_NOISE_GATE_RANGE, DEFAULT_NOISE_GATE_RANGE)
        editor.putFloat(KEY_NOISE_GATE_ATTACK, DEFAULT_NOISE_GATE_ATTACK)
        editor.putFloat(KEY_NOISE_GATE_RELEASE, DEFAULT_NOISE_GATE_RELEASE)
        editor.putBoolean(KEY_COMPRESSOR_ENABLED, DEFAULT_COMPRESSOR_ENABLED)
        editor.putFloat(KEY_COMPRESSOR_THRESHOLD, DEFAULT_COMPRESSOR_THRESHOLD)
        editor.putFloat(KEY_COMPRESSOR_RATIO, DEFAULT_COMPRESSOR_RATIO)
        editor.putFloat(KEY_COMPRESSOR_ATTACK, DEFAULT_COMPRESSOR_ATTACK)
        editor.putFloat(KEY_COMPRESSOR_RELEASE, DEFAULT_COMPRESSOR_RELEASE)
        editor.putFloat(KEY_COMPRESSOR_MAKEUP_GAIN, DEFAULT_COMPRESSOR_MAKEUP_GAIN)
        editor.putBoolean(KEY_LIMITER_ENABLED, DEFAULT_LIMITER_ENABLED)
        editor.putFloat(KEY_LIMITER_THRESHOLD, DEFAULT_LIMITER_THRESHOLD)
        editor.putFloat(KEY_LIMITER_ATTACK, DEFAULT_LIMITER_ATTACK)
        editor.putFloat(KEY_LIMITER_RELEASE, DEFAULT_LIMITER_RELEASE)
        editor.putBoolean(KEY_PRE_EQUALIZER_ENABLED, DEFAULT_PRE_EQUALIZER_ENABLED)
        editor.putBoolean(KEY_POST_EQUALIZER_ENABLED, DEFAULT_POST_EQUALIZER_ENABLED)

        for (i in DEFAULT_PRE_EQ_FREQUENCIES.indices) {
            editor.putInt(eqTypeKey(true, i), DEFAULT_EQ_TYPE)
            editor.putFloat(eqFreqKey(true, i), DEFAULT_PRE_EQ_FREQUENCIES[i])
            editor.putFloat(eqQKey(true, i), DEFAULT_EQ_Q)
            editor.putFloat(eqGainKey(true, i), DEFAULT_EQ_GAIN)
        }
        for (i in DEFAULT_POST_EQ_FREQUENCIES.indices) {
            editor.putInt(eqTypeKey(false, i), DEFAULT_EQ_TYPE)
            editor.putFloat(eqFreqKey(false, i), DEFAULT_POST_EQ_FREQUENCIES[i])
            editor.putFloat(eqQKey(false, i), DEFAULT_EQ_Q)
            editor.putFloat(eqGainKey(false, i), DEFAULT_EQ_GAIN)
        }

        editor.apply()
    }

    data class EqualizerBandSettings(
        val type: Int,
        val frequency: Float,
        val q: Float,
        val gain: Float
    )
}
