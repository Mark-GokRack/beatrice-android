package com.gokrack.beatriceapp

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatDelegate
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.lifecycle.ViewModelProvider
import androidx.viewpager2.widget.ViewPager2
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayoutMediator

class MainActivity : AppCompatActivity(), ActivityCompat.OnRequestPermissionsResultCallback {

    companion object {
        private val TAG = MainActivity::class.java.name
        private const val AUDIO_EFFECT_REQUEST = 0
    }

    private lateinit var viewModel: EngineStateViewModel

    override fun onCreate(savedInstanceState: Bundle?) {
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        SettingsManager.init(this)

        viewModel = ViewModelProvider(this)[EngineStateViewModel::class.java]
        viewModel.morphingWeights.value = SettingsManager.loadMorphingWeights()

        // ViewPager2 + TabLayout
        val viewPager = findViewById<ViewPager2>(R.id.view_pager)
        viewPager.adapter = MainPagerAdapter(this)

        val tabTitles = listOf(
            getString(R.string.tab_system),
            getString(R.string.tab_voice),
            getString(R.string.tab_basic),
            getString(R.string.tab_morphing)
        )
        TabLayoutMediator(findViewById(R.id.tab_layout), viewPager) { tab, position ->
            tab.text = tabTitles[position]
        }.attach()

        viewPager.setCurrentItem(1, false) // Set the initial tab to the first one (System)

        viewModel.statusText.value = getString(R.string.status_warning)

        beatriceEngine.setDefaultStreamValues(this)
        volumeControlStream = AudioManager.STREAM_MUSIC

        if (!isRecordPermissionGranted()) {
            requestRecordPermission()
        } else {
            startForegroundService()
        }

        onStartEngine()
    }

    private fun onStartEngine() {
        beatriceEngine.create(
            assets,
            getExternalFilesDir(null)?.absolutePath ?: filesDir.absolutePath
        )
        viewModel.isAAudioRecommended.value = beatriceEngine.isAAudioRecommended()

        // Apply initial settings from ViewModel defaults
        beatriceEngine.setAPI(viewModel.apiSelection.value ?: 0)
        beatriceEngine.setPerformanceMode(viewModel.performanceMode.value ?: 0)
        beatriceEngine.setAsyncMode(viewModel.isAsyncMode.value ?: true)
        applyPersistedUserSettingsToEngine()

        // Load model info if a model is already present
        val modelName = beatriceEngine.getModelName()
        if (modelName.isNotEmpty()) {
            viewModel.modelName.value = modelName
            val voiceNameList = ArrayList<String>()
            var voiceCount = 0
            for (i in 0 until 256) {
                val voiceName = beatriceEngine.getVoiceName(i)
                if (voiceName.isNotEmpty()) {
                    voiceNameList.add(voiceName)
                    voiceCount++
                } else break
            }
            // Notify ViewModel about morphing voices BEFORE appending VoiceMorphingMode
            viewModel.onMorphingModelLoaded(voiceCount, voiceNameList.toList())
            if( voiceCount > 1 ){
                voiceNameList.add( "Voice Morphing Mode");
            }
            viewModel.voiceNames.value = voiceNameList
        }
    }

    internal fun resetUserAdjustableSettings() {
        SettingsManager.resetAllToDefaults()
        viewModel.morphingWeights.value = SettingsManager.loadMorphingWeights()
        applyPersistedUserSettingsToEngine()
        viewModel.requestSettingsReset()
    }

    private fun applyPersistedUserSettingsToEngine() {
        beatriceEngine.setVoiceID(SettingsManager.loadVoiceId())
        beatriceEngine.setInputGain(SettingsManager.loadInputGain().toDouble())
        beatriceEngine.setOutputGain(SettingsManager.loadOutputGain().toDouble())
        beatriceEngine.setPitchShift(SettingsManager.loadPitchShift().toDouble())
        beatriceEngine.setFormantShift(SettingsManager.loadFormantShift().toDouble())
        beatriceEngine.setVQNumNeighbors(SettingsManager.loadVQNeighbors())
        beatriceEngine.setIntonationIntensity(SettingsManager.loadIntonationIntensity().toDouble())
        beatriceEngine.setPitchCorrection(SettingsManager.loadPitchCorrection().toDouble())
        beatriceEngine.setPitchCorrectionMode(SettingsManager.loadPitchCorrectionMode())
        beatriceEngine.setSourcePitchRange(
            SettingsManager.loadSourcePitchRangeMin().toDouble(),
            SettingsManager.loadSourcePitchRangeMax().toDouble()
        )
    }

    override fun onDestroy() {
        onStopEngine()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            startForegroundService(
                Intent(BeatriceForegroundService.ACTION_STOP, null, this, BeatriceForegroundService::class.java)
            )
        }
        super.onDestroy()
    }

    internal fun toggleEffect() {
        if (viewModel.isEngineRunning.value == true) {
            stopEffect()
        } else {
            beatriceEngine.setAPI(viewModel.apiSelection.value ?: 0)
            beatriceEngine.setPerformanceMode(viewModel.performanceMode.value ?: 0)
            beatriceEngine.setAsyncMode(viewModel.isAsyncMode.value ?: true)
            startEffect()
        }
    }

    private fun startEffect() {
        Log.d(TAG, "Attempting to start")
        val success = beatriceEngine.setEffectOn(true)
        if (success) {
            // Re-apply all morphing weights in case the stream lost them on restart
            viewModel.morphingWeights.value?.let { weights ->
                for (i in weights.indices) {
                    beatriceEngine.setSpeakerMorphingWeight(i, weights[i].toDouble())
                }
            }
            val sampleRate = beatriceEngine.getSampleRate()
            val framesPerBurst = beatriceEngine.getFramesPerBurst()
            viewModel.statusText.value = getString(R.string.status_playing) +
                "\nsampling frequency : ${sampleRate} Hz \t frame size: ${framesPerBurst} samples"
            viewModel.isEngineRunning.value = true
        } else {
            viewModel.statusText.value = getString(R.string.status_open_failed)
        }
    }

    private fun stopEffect() {
        Log.d(TAG, "Attempting to stop")
        beatriceEngine.setEffectOn(false)
        viewModel.statusText.value = getString(R.string.status_warning)
        viewModel.isEngineRunning.value = false
    }

    private fun onStopEngine() {
        if (viewModel.isEngineRunning.value == true) {
            beatriceEngine.setEffectOn(false)
            viewModel.isEngineRunning.value = false
        }
        beatriceEngine.delete()
    }

    private fun isRecordPermissionGranted() =
        ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) ==
                PackageManager.PERMISSION_GRANTED

    private fun requestRecordPermission() {
        ActivityCompat.requestPermissions(
            this, arrayOf(Manifest.permission.RECORD_AUDIO), AUDIO_EFFECT_REQUEST
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == AUDIO_EFFECT_REQUEST) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                viewModel.statusText.value = getString(R.string.status_touch_to_begin)
                startForegroundService()
            } else {
                viewModel.statusText.value = getString(R.string.status_record_audio_denied)
            }
        }
    }

    private fun startForegroundService() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            startForegroundService(
                Intent(BeatriceForegroundService.ACTION_START, null, this, BeatriceForegroundService::class.java)
            )
        }
    }
}
