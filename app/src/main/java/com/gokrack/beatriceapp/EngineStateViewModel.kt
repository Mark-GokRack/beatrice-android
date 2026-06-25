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
}
