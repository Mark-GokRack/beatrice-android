package com.gokrack.beatriceapp

import android.content.Context
import android.content.SharedPreferences
import java.util.Locale

/**
 * Manages preset slots for saving and loading multiple parameter configurations.
 * Each slot stores all SettingsManager parameters as a snapshot.
 */
object PresetManager {
    private const val PREFS_NAME = "beatrice_presets"
    private const val CURRENT_PRESET_INDEX = "current_preset_index"
    private const val PRESET_COUNT = 8

    private lateinit var prefs: SharedPreferences

    fun init(context: Context) {
        prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    }

    /**
     * Get list of preset names (e.g., "Preset 1", "Preset 2", ...)
     */
    fun getPresetNames(): List<String> {
        return (1..PRESET_COUNT).map { "Preset $it" }
    }

    /**
     * Get currently selected preset index
     */
    fun getCurrentPresetIndex(): Int {
        return prefs.getInt(CURRENT_PRESET_INDEX, 0)
    }

    /**
     * Save current SettingsManager state to the specified preset slot
     */
    fun savePreset(slotIndex: Int) {
        if (slotIndex < 0 || slotIndex >= PRESET_COUNT) return

        val editor = prefs.edit()
        val prefix = "preset_${slotIndex}_"

        // Voice and main parameters
        editor.putInt(prefix + "voice_id", SettingsManager.loadVoiceId())
        editor.putFloat(prefix + "input_gain", SettingsManager.loadInputGain())
        editor.putFloat(prefix + "output_gain", SettingsManager.loadOutputGain())
        editor.putFloat(prefix + "pitch_shift", SettingsManager.loadPitchShift())
        editor.putFloat(prefix + "formant_shift", SettingsManager.loadFormantShift())
        editor.putInt(prefix + "vq_neighbors", SettingsManager.loadVQNeighbors())
        editor.putFloat(prefix + "intonation_intensity", SettingsManager.loadIntonationIntensity())
        editor.putFloat(prefix + "pitch_correction", SettingsManager.loadPitchCorrection())
        editor.putInt(prefix + "pitch_correction_mode", SettingsManager.loadPitchCorrectionMode())
        editor.putFloat(prefix + "source_pitch_range_min", SettingsManager.loadSourcePitchRangeMin())
        editor.putFloat(prefix + "source_pitch_range_max", SettingsManager.loadSourcePitchRangeMax())

        // Morphing weights
        val morphingWeights = SettingsManager.loadMorphingWeights()
        val morphingSerialized = morphingWeights.joinToString(";") { String.format(Locale.US, "%.6f", it) }
        editor.putString(prefix + "morphing_weights", morphingSerialized)

        // Amplifier
        editor.putBoolean(prefix + "amplifier_enabled", SettingsManager.loadAmplifierEnabled())
        editor.putFloat(prefix + "amplifier_gain", SettingsManager.loadAmplifierGain())

        // Noise Gate
        editor.putBoolean(prefix + "noise_gate_enabled", SettingsManager.loadNoiseGateEnabled())
        editor.putFloat(prefix + "noise_gate_threshold", SettingsManager.loadNoiseGateThreshold())
        editor.putFloat(prefix + "noise_gate_range", SettingsManager.loadNoiseGateRange())
        editor.putFloat(prefix + "noise_gate_attack", SettingsManager.loadNoiseGateAttack())
        editor.putFloat(prefix + "noise_gate_release", SettingsManager.loadNoiseGateRelease())

        // Compressor
        editor.putBoolean(prefix + "compressor_enabled", SettingsManager.loadCompressorEnabled())
        editor.putFloat(prefix + "compressor_threshold", SettingsManager.loadCompressorThreshold())
        editor.putFloat(prefix + "compressor_ratio", SettingsManager.loadCompressorRatio())
        editor.putFloat(prefix + "compressor_attack", SettingsManager.loadCompressorAttack())
        editor.putFloat(prefix + "compressor_release", SettingsManager.loadCompressorRelease())
        editor.putFloat(prefix + "compressor_makeup_gain", SettingsManager.loadCompressorMakeupGain())

        // Limiter
        editor.putBoolean(prefix + "limiter_enabled", SettingsManager.loadLimiterEnabled())
        editor.putFloat(prefix + "limiter_threshold", SettingsManager.loadLimiterThreshold())
        editor.putFloat(prefix + "limiter_attack", SettingsManager.loadLimiterAttack())
        editor.putFloat(prefix + "limiter_release", SettingsManager.loadLimiterRelease())

        // RNNoise
        editor.putBoolean(prefix + "rnnoise_enabled", SettingsManager.loadRnnoiseEnabled())

        // Equalizers
        editor.putBoolean(prefix + "pre_equalizer_enabled", SettingsManager.loadPreEqualizerEnabled())
        editor.putBoolean(prefix + "post_equalizer_enabled", SettingsManager.loadPostEqualizerEnabled())

        // Pre-EQ (3 bands)
        for (i in 0 until 3) {
            val bandSettings = SettingsManager.loadEqualizerBand(true, i)
            editor.putInt(prefix + "pre_eq_band_${i}_type", bandSettings.type)
            editor.putFloat(prefix + "pre_eq_band_${i}_freq", bandSettings.frequency)
            editor.putFloat(prefix + "pre_eq_band_${i}_q", bandSettings.q)
            editor.putFloat(prefix + "pre_eq_band_${i}_gain", bandSettings.gain)
        }

        // Post-EQ (5 bands)
        for (i in 0 until 5) {
            val bandSettings = SettingsManager.loadEqualizerBand(false, i)
            editor.putInt(prefix + "post_eq_band_${i}_type", bandSettings.type)
            editor.putFloat(prefix + "post_eq_band_${i}_freq", bandSettings.frequency)
            editor.putFloat(prefix + "post_eq_band_${i}_q", bandSettings.q)
            editor.putFloat(prefix + "post_eq_band_${i}_gain", bandSettings.gain)
        }

        editor.putInt(CURRENT_PRESET_INDEX, slotIndex)
        editor.apply()
    }

    /**
     * Load preset from the specified slot into SettingsManager
     */
    fun loadPreset(slotIndex: Int) {
        if (slotIndex < 0 || slotIndex >= PRESET_COUNT) return

        val prefix = "preset_${slotIndex}_"

        // Voice and main parameters
        SettingsManager.saveVoiceId(prefs.getInt(prefix + "voice_id", SettingsManager.DEFAULT_VOICE_ID))
        SettingsManager.saveInputGain(prefs.getFloat(prefix + "input_gain", SettingsManager.DEFAULT_INPUT_GAIN))
        SettingsManager.saveOutputGain(prefs.getFloat(prefix + "output_gain", SettingsManager.DEFAULT_OUTPUT_GAIN))
        SettingsManager.savePitchShift(prefs.getFloat(prefix + "pitch_shift", SettingsManager.DEFAULT_PITCH_SHIFT))
        SettingsManager.saveFormantShift(prefs.getFloat(prefix + "formant_shift", SettingsManager.DEFAULT_FORMANT_SHIFT))
        SettingsManager.saveVQNeighbors(prefs.getInt(prefix + "vq_neighbors", SettingsManager.DEFAULT_VQ_NEIGHBORS))
        SettingsManager.saveIntonationIntensity(prefs.getFloat(prefix + "intonation_intensity", SettingsManager.DEFAULT_INTONATION_INTENSITY))
        SettingsManager.savePitchCorrection(prefs.getFloat(prefix + "pitch_correction", SettingsManager.DEFAULT_PITCH_CORRECTION))
        SettingsManager.savePitchCorrectionMode(prefs.getInt(prefix + "pitch_correction_mode", SettingsManager.DEFAULT_PITCH_CORRECTION_MODE))
        val min = prefs.getFloat(prefix + "source_pitch_range_min", SettingsManager.DEFAULT_SOURCE_PITCH_MIN)
        val max = prefs.getFloat(prefix + "source_pitch_range_max", SettingsManager.DEFAULT_SOURCE_PITCH_MAX)
        SettingsManager.saveSourcePitchRange(min, max)

        // Morphing weights
        val morphingSerialized = prefs.getString(prefix + "morphing_weights", null)
        if (morphingSerialized != null) {
            val parsed = morphingSerialized.split(";").mapNotNull { it.toFloatOrNull() }
            if (parsed.size == 256) {  // Morphing weights are always 256-element array
                SettingsManager.saveMorphingWeights(parsed.toFloatArray())
            }else{
                // If morphing weights are not loaded, reset to default
                SettingsManager.resetMorphingWeights()
            }
        }else{
            // If morphing weights are not loaded, reset to default
            SettingsManager.resetMorphingWeights()
        }

        // Amplifier
        SettingsManager.saveAmplifierEnabled(prefs.getBoolean(prefix + "amplifier_enabled", SettingsManager.DEFAULT_AMPLIFIER_ENABLED))
        SettingsManager.saveAmplifierGain(prefs.getFloat(prefix + "amplifier_gain", SettingsManager.DEFAULT_AMPLIFIER_GAIN))

        // Noise Gate
        SettingsManager.saveNoiseGateEnabled(prefs.getBoolean(prefix + "noise_gate_enabled", SettingsManager.DEFAULT_NOISE_GATE_ENABLED))
        SettingsManager.saveNoiseGateThreshold(prefs.getFloat(prefix + "noise_gate_threshold", SettingsManager.DEFAULT_NOISE_GATE_THRESHOLD))
        SettingsManager.saveNoiseGateRange(prefs.getFloat(prefix + "noise_gate_range", SettingsManager.DEFAULT_NOISE_GATE_RANGE))
        SettingsManager.saveNoiseGateAttack(prefs.getFloat(prefix + "noise_gate_attack", SettingsManager.DEFAULT_NOISE_GATE_ATTACK))
        SettingsManager.saveNoiseGateRelease(prefs.getFloat(prefix + "noise_gate_release", SettingsManager.DEFAULT_NOISE_GATE_RELEASE))

        // Compressor
        SettingsManager.saveCompressorEnabled(prefs.getBoolean(prefix + "compressor_enabled", SettingsManager.DEFAULT_COMPRESSOR_ENABLED))
        SettingsManager.saveCompressorThreshold(prefs.getFloat(prefix + "compressor_threshold", SettingsManager.DEFAULT_COMPRESSOR_THRESHOLD))
        SettingsManager.saveCompressorRatio(prefs.getFloat(prefix + "compressor_ratio", SettingsManager.DEFAULT_COMPRESSOR_RATIO))
        SettingsManager.saveCompressorAttack(prefs.getFloat(prefix + "compressor_attack", SettingsManager.DEFAULT_COMPRESSOR_ATTACK))
        SettingsManager.saveCompressorRelease(prefs.getFloat(prefix + "compressor_release", SettingsManager.DEFAULT_COMPRESSOR_RELEASE))
        SettingsManager.saveCompressorMakeupGain(prefs.getFloat(prefix + "compressor_makeup_gain", SettingsManager.DEFAULT_COMPRESSOR_MAKEUP_GAIN))

        // Limiter
        SettingsManager.saveLimiterEnabled(prefs.getBoolean(prefix + "limiter_enabled", SettingsManager.DEFAULT_LIMITER_ENABLED))
        SettingsManager.saveLimiterThreshold(prefs.getFloat(prefix + "limiter_threshold", SettingsManager.DEFAULT_LIMITER_THRESHOLD))
        SettingsManager.saveLimiterAttack(prefs.getFloat(prefix + "limiter_attack", SettingsManager.DEFAULT_LIMITER_ATTACK))
        SettingsManager.saveLimiterRelease(prefs.getFloat(prefix + "limiter_release", SettingsManager.DEFAULT_LIMITER_RELEASE))

        // RNNoise
        SettingsManager.saveRnnoiseEnabled(prefs.getBoolean(prefix + "rnnoise_enabled", SettingsManager.DEFAULT_RNNOISE_ENABLED))

        // Equalizers
        SettingsManager.savePreEqualizerEnabled(prefs.getBoolean(prefix + "pre_equalizer_enabled", SettingsManager.DEFAULT_PRE_EQUALIZER_ENABLED))
        SettingsManager.savePostEqualizerEnabled(prefs.getBoolean(prefix + "post_equalizer_enabled", SettingsManager.DEFAULT_POST_EQUALIZER_ENABLED))

        // Pre-EQ (3 bands: 100Hz, 1kHz, 10kHz)
        for (i in 0 until 3) {
            val type = prefs.getInt(prefix + "pre_eq_band_${i}_type", SettingsManager.DEFAULT_EQ_TYPE)
            val frequencies = floatArrayOf(100f, 1000f, 10000f)
            val freq = prefs.getFloat(prefix + "pre_eq_band_${i}_freq", frequencies.getOrElse(i) { 1000f })
            val q = prefs.getFloat(prefix + "pre_eq_band_${i}_q", SettingsManager.DEFAULT_EQ_Q)
            val gain = prefs.getFloat(prefix + "pre_eq_band_${i}_gain", SettingsManager.DEFAULT_EQ_GAIN)
            SettingsManager.saveEqualizerBand(true, i, type, freq, q, gain)
        }

        // Post-EQ (5 bands: 30Hz, 100Hz, 300Hz, 1kHz, 10kHz)
        for (i in 0 until 5) {
            val type = prefs.getInt(prefix + "post_eq_band_${i}_type", SettingsManager.DEFAULT_EQ_TYPE)
            val frequencies = floatArrayOf(30f, 100f, 300f, 1000f, 10000f)
            val freq = prefs.getFloat(prefix + "post_eq_band_${i}_freq", frequencies.getOrElse(i) { 1000f })
            val q = prefs.getFloat(prefix + "post_eq_band_${i}_q", SettingsManager.DEFAULT_EQ_Q)
            val gain = prefs.getFloat(prefix + "post_eq_band_${i}_gain", SettingsManager.DEFAULT_EQ_GAIN)
            SettingsManager.saveEqualizerBand(false, i, type, freq, q, gain)
        }

        prefs.edit().putInt(CURRENT_PRESET_INDEX, slotIndex).apply()
    }
}
