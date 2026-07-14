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

    fun loadMorphingWeights(): FloatArray {
        val serialized = prefs.getString(KEY_MORPHING_WEIGHTS, null) ?: return DEFAULT_MORPHING_WEIGHTS.copyOf()
        val parsed = serialized.split(";").mapNotNull { it.toFloatOrNull() }
        return if (parsed.size == DEFAULT_MORPHING_WEIGHTS.size) {
            parsed.toFloatArray()
        } else {
            DEFAULT_MORPHING_WEIGHTS.copyOf()
        }
    }

    fun resetAllToDefaults() {
        prefs.edit()
            .clear()
            .putFloat(KEY_INPUT_GAIN, DEFAULT_INPUT_GAIN)
            .putFloat(KEY_OUTPUT_GAIN, DEFAULT_OUTPUT_GAIN)
            .putFloat(KEY_PITCH_SHIFT, DEFAULT_PITCH_SHIFT)
            .putFloat(KEY_FORMANT_SHIFT, DEFAULT_FORMANT_SHIFT)
            .putInt(KEY_VQ_NEIGHBORS, DEFAULT_VQ_NEIGHBORS)
            .putFloat(KEY_INTONATION_INTENSITY, DEFAULT_INTONATION_INTENSITY)
            .putFloat(KEY_PITCH_CORRECTION, DEFAULT_PITCH_CORRECTION)
            .putInt(KEY_PITCH_CORRECTION_MODE, DEFAULT_PITCH_CORRECTION_MODE)
            .putFloat(KEY_SOURCE_PITCH_RANGE_MIN, DEFAULT_SOURCE_PITCH_MIN)
            .putFloat(KEY_SOURCE_PITCH_RANGE_MAX, DEFAULT_SOURCE_PITCH_MAX)
            .putInt(KEY_VOICE_ID, DEFAULT_VOICE_ID)
            .putString(KEY_MORPHING_WEIGHTS, DEFAULT_MORPHING_WEIGHTS.joinToString(";") { it.toString() })
            .apply()
    }
}
