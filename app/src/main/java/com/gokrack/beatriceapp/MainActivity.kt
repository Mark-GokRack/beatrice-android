package com.gokrack.beatriceapp

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.ArrayAdapter
import android.widget.ImageButton
import android.widget.Spinner
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.app.ActivityCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
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
        
        // WindowInsets API: ノッチ対応
        WindowCompat.setDecorFitsSystemWindows(window, false)
        
        setContentView(R.layout.activity_main)

        SettingsManager.init(this)
        PresetManager.init(this)

        viewModel = ViewModelProvider(this)[EngineStateViewModel::class.java]
        viewModel.morphingWeights.value = SettingsManager.loadMorphingWeights()

        // ステータスバーの下にプリセットバーがくるよう高さとパディングを調整
        val presetBar = findViewById<android.widget.LinearLayout>(R.id.preset_bar)
        val baseHeightPx = (36f * resources.displayMetrics.density).toInt()
        ViewCompat.setOnApplyWindowInsetsListener(presetBar) { view, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            // 高さを56dp + ステータスバー高さに設定
            view.layoutParams.height = baseHeightPx + systemBars.top
            view.layoutParams = view.layoutParams
            // 上部パディングでノッチを回避
            view.setPadding(view.paddingLeft, systemBars.top, view.paddingRight, view.paddingBottom)
            insets
        }

        // プリセットバー初期化
        setupPresetBar()

        // ViewPager2 + TabLayout
        val viewPager = findViewById<ViewPager2>(R.id.view_pager)
        viewPager.adapter = MainPagerAdapter(this)

        val tabTitles = listOf(
            getString(R.string.tab_system),
            getString(R.string.tab_model),
            getString(R.string.tab_params),
            getString(R.string.tab_effector),
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

    private fun setupPresetBar() {
        val presetSpinner = findViewById<Spinner>(R.id.preset_spinner)
        val saveButton = findViewById<ImageButton>(R.id.preset_save_button)
        val resetButton = findViewById<ImageButton>(R.id.preset_reset_button)

        // プリセット一覧を取得
        val presetNames = PresetManager.getPresetNames()
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, presetNames)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        presetSpinner.adapter = adapter

        // 現在のプリセットを選択状態に
        val currentPresetIndex = PresetManager.getCurrentPresetIndex()
        presetSpinner.setSelection(currentPresetIndex)

        // プリセット選択時
        presetSpinner.onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: android.view.View?, position: Int, id: Long) {
                PresetManager.loadPreset(position)
                applyPersistedUserSettingsToEngine()
                viewModel.morphingWeights.value = SettingsManager.loadMorphingWeights()
                viewModel.requestSettingsReset()
            }

            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {}
        }

        // 保存ボタン
        saveButton.setOnClickListener {
            val currentIndex = presetSpinner.selectedItemPosition
            PresetManager.savePreset(currentIndex)
            android.widget.Toast.makeText(
                this,
                getString(R.string.preset_saved, presetNames[currentIndex]),
                android.widget.Toast.LENGTH_SHORT
            ).show()
        }

        // リセットボタン
        resetButton.setOnClickListener {
            android.app.AlertDialog.Builder(this)
                .setTitle(R.string.preset_reset_confirm_title)
                .setMessage(R.string.preset_reset_confirm_message)
                .setPositiveButton(R.string.preset_reset) { _, _ ->
                    resetUserAdjustableSettings()
                }
                .setNegativeButton(android.R.string.cancel, null)
                .show()
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
        applyEffectorSettingsToEngine()
    }

    private fun applyEffectorSettingsToEngine() {
        beatriceEngine.setAmplifierEnabled(SettingsManager.loadAmplifierEnabled())
        beatriceEngine.setAmplifierGain(SettingsManager.loadAmplifierGain().toDouble())

        beatriceEngine.setNoiseGateEnabled(SettingsManager.loadNoiseGateEnabled())
        beatriceEngine.setNoiseGateThreshold(SettingsManager.loadNoiseGateThreshold().toDouble())
        beatriceEngine.setNoiseGateRange(SettingsManager.loadNoiseGateRange().toDouble())
        beatriceEngine.setNoiseGateAttack(SettingsManager.loadNoiseGateAttack().toDouble())
        beatriceEngine.setNoiseGateRelease(SettingsManager.loadNoiseGateRelease().toDouble())

        beatriceEngine.setCompressorEnabled(SettingsManager.loadCompressorEnabled())
        beatriceEngine.setCompressorThreshold(SettingsManager.loadCompressorThreshold().toDouble())
        beatriceEngine.setCompressorRatio(SettingsManager.loadCompressorRatio().toDouble())
        beatriceEngine.setCompressorAttack(SettingsManager.loadCompressorAttack().toDouble())
        beatriceEngine.setCompressorRelease(SettingsManager.loadCompressorRelease().toDouble())
        beatriceEngine.setCompressorMakeupGain(SettingsManager.loadCompressorMakeupGain().toDouble())

        beatriceEngine.setLimiterEnabled(SettingsManager.loadLimiterEnabled())
        beatriceEngine.setLimiterThreshold(SettingsManager.loadLimiterThreshold().toDouble())
        beatriceEngine.setLimiterAttack(SettingsManager.loadLimiterAttack().toDouble())
        beatriceEngine.setLimiterRelease(SettingsManager.loadLimiterRelease().toDouble())

        beatriceEngine.setPreEqualizerEnabled(SettingsManager.loadPreEqualizerEnabled())
        for (i in SettingsManager.DEFAULT_PRE_EQ_FREQUENCIES.indices) {
            val band = SettingsManager.loadEqualizerBand(true, i)
            applyEqualizerBandToEngine(true, i, band)
        }

        beatriceEngine.setPostEqualizerEnabled(SettingsManager.loadPostEqualizerEnabled())
        for (i in SettingsManager.DEFAULT_POST_EQ_FREQUENCIES.indices) {
            val band = SettingsManager.loadEqualizerBand(false, i)
            applyEqualizerBandToEngine(false, i, band)
        }
    }

    private fun applyEqualizerBandToEngine(isPre: Boolean, band: Int, settings: SettingsManager.EqualizerBandSettings) {
        val index = band
        when (settings.type) {
            0 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsPeaking(index, settings.frequency.toDouble(), settings.q.toDouble(), settings.gain.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsPeaking(index, settings.frequency.toDouble(), settings.q.toDouble(), settings.gain.toDouble())
            }
            1 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsLowpass(index, settings.frequency.toDouble(), settings.q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsLowpass(index, settings.frequency.toDouble(), settings.q.toDouble())
            }
            2 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsHighpass(index, settings.frequency.toDouble(), settings.q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsHighpass(index, settings.frequency.toDouble(), settings.q.toDouble())
            }
            3 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsLowShelf(index, settings.frequency.toDouble(), settings.q.toDouble(), settings.gain.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsLowShelf(index, settings.frequency.toDouble(), settings.q.toDouble(), settings.gain.toDouble())
            }
            4 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsHighShelf(index, settings.frequency.toDouble(), settings.q.toDouble(), settings.gain.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsHighShelf(index, settings.frequency.toDouble(), settings.q.toDouble(), settings.gain.toDouble())
            }
            5 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsNotch(index, settings.frequency.toDouble(), settings.q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsNotch(index, settings.frequency.toDouble(), settings.q.toDouble())
            }
            6 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsAllpass(index, settings.frequency.toDouble(), settings.q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsAllpass(index, settings.frequency.toDouble(), settings.q.toDouble())
            }
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
