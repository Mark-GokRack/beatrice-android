package com.gokrack.beatriceapp

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

class EngineStateViewModel : ViewModel() {
    val isEngineRunning = MutableLiveData(false)
    val isAAudioRecommended = MutableLiveData(true)
    val apiSelection = MutableLiveData(0)       // 0 = AAudio, 1 = OpenSL ES
    val performanceMode = MutableLiveData(0)    // 0 = LowLatency, 1 = Normal, 2 = PowerSaving
    val isAsyncMode = MutableLiveData(true)
    val modelName = MutableLiveData("")
    val voiceNames = MutableLiveData<List<String>>(emptyList())

    // Morphing tab state
    val morphingWeights = MutableLiveData(FloatArray(256) { 0f })
    val morphingVoiceNames = MutableLiveData<List<String>>(emptyList())
    val morphingDescriptionTrigger = MutableLiveData<Unit>()

    /**
     * Called on app start and model load. Enforces rules for morphing weights:
     * - Zeros out any weights for voices beyond voiceCount
     * - For model version >= 2, zeros out weights from highest index downward
     *   until the non-zero count is at most 8
     */
    fun onMorphingModelLoaded(voiceCount: Int, voiceNames: List<String>) {
        val weights = (morphingWeights.value ?: FloatArray(256)).clone()
        val modelVersion = beatriceEngine.getModelVersion()

        // Zero out voices beyond voiceCount
        for (i in voiceCount until 256) {
            if (weights[i] != 0f) {
                weights[i] = 0f
                beatriceEngine.setSpeakerMorphingWeight(i, 0.0)
            }
        }

        // Enforce 8 non-zero limit for model version >= 2
        if (modelVersion >= 2) {
            var nonZeroCount = (0 until voiceCount).count { weights[it] != 0f }
            for (i in (voiceCount - 1) downTo 0) {
                if (nonZeroCount <= 8) break
                if (weights[i] != 0f) {
                    weights[i] = 0f
                    beatriceEngine.setSpeakerMorphingWeight(i, 0.0)
                    nonZeroCount--
                }
            }
        }

        // If all weights within valid voices are zero, default the first voice to 1.0
        if (voiceCount > 0 && (0 until voiceCount).all { weights[it] == 0f }) {
            weights[0] = 1.0f
            beatriceEngine.setSpeakerMorphingWeight(0, 1.0)
        }

        morphingWeights.postValue(weights)
        morphingVoiceNames.postValue(voiceNames)
    }
}
