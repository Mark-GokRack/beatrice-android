package com.gokrack.beatriceapp

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
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
    private lateinit var statusText: TextView
    private lateinit var toggleEffectButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        viewModel = ViewModelProvider(this)[EngineStateViewModel::class.java]

        statusText = findViewById(R.id.status_view_text)
        toggleEffectButton = findViewById(R.id.button_toggle_effect)
        toggleEffectButton.setOnClickListener { toggleEffect() }

        // ViewPager2 + TabLayout
        val viewPager = findViewById<ViewPager2>(R.id.view_pager)
        viewPager.adapter = MainPagerAdapter(this)

        val tabTitles = listOf(
            getString(R.string.tab_system),
            getString(R.string.tab_voice),
            getString(R.string.tab_basic),
            getString(R.string.tab_advanced)
        )
        TabLayoutMediator(findViewById(R.id.tab_layout), viewPager) { tab, position ->
            tab.text = tabTitles[position]
        }.attach()

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

        // Load model info if a model is already present
        val modelName = beatriceEngine.getModelName()
        if (modelName.isNotEmpty()) {
            viewModel.modelName.value = modelName
            val voiceNameList = ArrayList<String>()
            for (i in 0 until 256) {
                val voiceName = beatriceEngine.getVoiceName(i)
                if (voiceName.isNotEmpty()) voiceNameList.add(voiceName) else break
            }
            viewModel.voiceNames.value = voiceNameList
        }
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

    private fun toggleEffect() {
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
            val sampleRate = beatriceEngine.getSampleRate()
            val framesPerBurst = beatriceEngine.getFramesPerBurst()
            statusText.text = getString(R.string.status_playing) +
                "\nsampling frequency : ${sampleRate} Hz \t frame size: ${framesPerBurst} samples"
            toggleEffectButton.setText(R.string.stop_effect)
            viewModel.isEngineRunning.value = true
        } else {
            statusText.setText(R.string.status_open_failed)
        }
    }

    private fun stopEffect() {
        Log.d(TAG, "Attempting to stop")
        beatriceEngine.setEffectOn(false)
        statusText.setText(R.string.status_warning)
        toggleEffectButton.setText(R.string.start_effect)
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
                statusText.setText(R.string.status_touch_to_begin)
                startForegroundService()
            } else {
                statusText.setText(R.string.status_record_audio_denied)
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
