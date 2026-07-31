package com.gokrack.beatriceapp

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

class EngineStateViewModel : ViewModel() {
    companion object {
        const val MORPHING_WEIGHT_SIZE = 256
    }

    val isEngineRunning = MutableLiveData(false)
    val isAAudioRecommended = MutableLiveData(true)
    val statusText = MutableLiveData("")
    val apiSelection = MutableLiveData(0)                    // 0 = AAudio, 1 = OpenSL ES
    val performanceMode = MutableLiveData(0)                 // 0 = LowLatency, 1 = Normal, 2 = PowerSaving
    val isAsyncMode = MutableLiveData(true)
    val isVoiceCommunicationMode = MutableLiveData(false)
    val modelName = MutableLiveData("")
    val voiceNames = MutableLiveData<List<String>>(emptyList())

    // Morphing tab state
    val morphingWeights = MutableLiveData(FloatArray(MORPHING_WEIGHT_SIZE) { 0f })
    val morphingVoiceNames = MutableLiveData<List<String>>(emptyList())
    val morphingDescriptionTrigger = MutableLiveData<Unit>()
    val settingsResetTrigger = MutableLiveData(0)

    fun requestSettingsReset() {
        settingsResetTrigger.value = (settingsResetTrigger.value ?: 0) + 1
    }

    fun updateMorphingWeight(index: Int, value: Float): FloatArray {
        require(index in 0 until MORPHING_WEIGHT_SIZE) { "index out of range: $index" }

        val base = morphingWeights.value ?: FloatArray(MORPHING_WEIGHT_SIZE) { 0f }
        val updated = base.copyOf()
        updated[index] = value
        morphingWeights.value = updated
        return updated
    }

    /**
     * Called on app start and model load. Enforces rules for morphing weights:
     * - Zeros out any weights for voices beyond voiceCount
     * - For model version >= 2, zeros out weights from highest index downward
     *   until the non-zero count is at most 8
     */
    fun onMorphingModelLoaded(voiceCount: Int, voiceNames: List<String>) {
        val weights = (morphingWeights.value ?: FloatArray(MORPHING_WEIGHT_SIZE)).clone()
        val modelVersion = beatriceEngine.getModelVersion()

        // Zero out voices beyond voiceCount
        for (i in voiceCount until MORPHING_WEIGHT_SIZE) {
            if (weights[i] != 0f) {
                weights[i] = 0f
            }
        }

        // Enforce 8 non-zero limit for model version >= 2
        if (modelVersion >= 2) {
            var nonZeroCount = (0 until voiceCount).count { weights[it] != 0f }
            for (i in (voiceCount - 1) downTo 0) {
                if (nonZeroCount <= 8) break
                if (weights[i] != 0f) {
                    weights[i] = 0f
                    nonZeroCount--
                }
            }
        }

        morphingWeights.postValue(weights)
        beatriceEngine.setSpeakerMorphingWeights(weights)
        morphingVoiceNames.postValue(voiceNames)
    }
}
