package com.gokrack.beatriceapp

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.*
import androidx.core.app.ActivityCompat
import com.google.oboe.samples.audio_device.AudioDeviceListEntry
import com.google.oboe.samples.audio_device.AudioDeviceSpinner
import android.net.Uri
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.documentfile.provider.DocumentFile
import java.io.File
import java.io.FileOutputStream
import android.provider.DocumentsContract
import androidx.activity.result.ActivityResultLauncher

class MainActivity : AppCompatActivity(), ActivityCompat.OnRequestPermissionsResultCallback {
    
    companion object {
        private val TAG = MainActivity::class.java.name
        private const val AUDIO_EFFECT_REQUEST = 0
        private const val OBOE_API_AAUDIO = 0
        private const val OBOE_API_OPENSL_ES = 1
    }

    private lateinit var statusText: TextView
    private lateinit var toggleEffectButton: Button
    private lateinit var recordingDeviceSpinner: AudioDeviceSpinner
    private lateinit var playbackDeviceSpinner: AudioDeviceSpinner
    private lateinit var voiceSpinner: Spinner
    private lateinit var pitchShiftSlider: com.google.android.material.slider.Slider
    private lateinit var formantShiftSlider: com.google.android.material.slider.Slider
    private lateinit var inputGainSlider: com.google.android.material.slider.Slider
    private lateinit var outputGainSlider: com.google.android.material.slider.Slider

    private var performanceMode = 0
    private var isAsyncMode = false

    private var isPlaying = false
    private var apiSelection = OBOE_API_AAUDIO
    private var mAAudioRecommended = true

    private var modelPickerLauncher: ActivityResultLauncher<Intent> = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            val treeUri = result.data?.data ?: return@registerForActivityResult
            // 永続的なアクセス許可を取得（必要に応じて）
            contentResolver.takePersistableUriPermission(
                treeUri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
            )

            // treeUri を使って DocumentFile を取得
            val rootDir = DocumentFile.fromTreeUri(this, treeUri)
            val tomlFiles = rootDir?.listFiles()?.filter { file ->
                file.isFile && file.name?.endsWith(".toml", ignoreCase = true) == true
            } ?: emptyList()

            if (tomlFiles.isNotEmpty()) {
                //Log.d("TOML", "Found ${tomlFiles.size} TOML file(s):")
                //tomlFiles.forEach { Log.d("TOML", it.name ?: "Unnamed") }
                // 例: 最初の toml ファイルを処理
                copyModelFilesToExtDir(tomlFiles.first().uri)
            } else {
                Toast.makeText(this, "TOMLファイルを含むフォルダを選択してください", Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusText = findViewById(R.id.status_view_text)
        toggleEffectButton = findViewById(R.id.button_toggle_effect)
        toggleEffectButton.setOnClickListener { toggleEffect() }
        toggleEffectButton.text = getString(R.string.start_effect)

        recordingDeviceSpinner = findViewById(R.id.recording_devices_spinner)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            recordingDeviceSpinner.setDirectionType(AudioManager.GET_DEVICES_INPUTS)
            recordingDeviceSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(parent: AdapterView<*>, view: View?, position: Int, id: Long) {
                    beatriceEngine.setRecordingDeviceId(getRecordingDeviceId())
                }

                override fun onNothingSelected(parent: AdapterView<*>) {}
            }
        }

        playbackDeviceSpinner = findViewById(R.id.playback_devices_spinner)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            playbackDeviceSpinner.setDirectionType(AudioManager.GET_DEVICES_OUTPUTS)
            playbackDeviceSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(parent: AdapterView<*>, view: View?, position: Int, id: Long) {
                    beatriceEngine.setPlaybackDeviceId(getPlaybackDeviceId())
                }

                override fun onNothingSelected(parent: AdapterView<*>) {}
            }
        }

        val model_select_button = findViewById<Button>(R.id.button_model_select)
        model_select_button.setOnClickListener {
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).apply {
                flags = Intent.FLAG_GRANT_READ_URI_PERMISSION or
                        Intent.FLAG_GRANT_WRITE_URI_PERMISSION or
                        Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
            }
            modelPickerLauncher.launch(intent)
        }

        voiceSpinner = findViewById(R.id.voice_select_spinner)
        voiceSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>, view: View?, position: Int, id: Long) {
                beatriceEngine.setVoiceID(id.toInt())
            }
            override fun onNothingSelected(parent: AdapterView<*>) {}
        }

        pitchShiftSlider = findViewById(R.id.pitch_shift_slider)
        pitchShiftSlider.addOnChangeListener { slider, value, fromUser ->
            beatriceEngine.setPitchShift(value)
        }

        formantShiftSlider = findViewById(R.id.formant_shift_slider)
        formantShiftSlider.addOnChangeListener { slider, value, fromUser ->
            beatriceEngine.setFormantShift(value)
        }

        inputGainSlider = findViewById(R.id.input_gain_slider)
        inputGainSlider.addOnChangeListener { slider, value, fromUser ->
            beatriceEngine.setInputGain(value)
        }

        outputGainSlider = findViewById(R.id.output_gain_slider)
        outputGainSlider.addOnChangeListener { slider, value, fromUser ->
            beatriceEngine.setOutputGain(value)
        }

        findViewById<RadioGroup>(R.id.apiSelectionGroup).check(R.id.aaudioButton)
        findViewById<RadioButton>(R.id.aaudioButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                apiSelection = OBOE_API_AAUDIO
                setSpinnersEnabled(true)
                EnablePerformanceModeUI(true)
                EnableAsyncModeUI(true)
            }
        }
        findViewById<RadioButton>(R.id.slesButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                apiSelection = OBOE_API_OPENSL_ES
                setSpinnersEnabled(false)
                EnablePerformanceModeUI(false)
                EnableAsyncModeUI(false)
            }
        }
        
        findViewById<RadioGroup>(R.id.LatencySelectionGroup).check(R.id.LowLatencyButton)
        findViewById<RadioButton>(R.id.LowLatencyButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                performanceMode = 0
            }
        }
        findViewById<RadioButton>(R.id.NormalLatencyButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                performanceMode = 1
            } 
        }
        findViewById<RadioButton>(R.id.PowerSavingButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                performanceMode = 2
            }
        }

        findViewById<CheckBox>(R.id.asyncProcessingCheckbox).setOnCheckedChangeListener { buttonView, isChecked ->
            isAsyncMode = isChecked
            if (isAsyncMode) {
                performanceMode = 0
                findViewById<RadioGroup>(R.id.LatencySelectionGroup).check(
                    R.id.LowLatencyButton
                )
                EnablePerformanceModeUI(false)
            } else {
                EnablePerformanceModeUI(true)
            }            
        }
        findViewById<CheckBox>(R.id.asyncProcessingCheckbox).setChecked(true)

        beatriceEngine.setDefaultStreamValues(this)
        volumeControlStream = AudioManager.STREAM_MUSIC

        if (!isRecordPermissionGranted()) {
            requestRecordPermission()
        } else {
            startForegroundService()
        }

        onStartTest()
    }

    private fun EnableAudioApiUI(enable: Boolean) {
        if (apiSelection == OBOE_API_AAUDIO && !mAAudioRecommended) {
            apiSelection = OBOE_API_OPENSL_ES
        }

        findViewById<RadioButton>(R.id.slesButton).isEnabled = enable
        findViewById<RadioButton>(R.id.aaudioButton).isEnabled = mAAudioRecommended && enable

        findViewById<RadioGroup>(R.id.apiSelectionGroup).check(
            if (apiSelection == OBOE_API_AAUDIO) R.id.aaudioButton else R.id.slesButton
        )
        setSpinnersEnabled(enable)
    }

    private fun EnablePerformanceModeUI(enable: Boolean) {

        findViewById<RadioButton>(R.id.LowLatencyButton).isEnabled = enable
        findViewById<RadioButton>(R.id.NormalLatencyButton).isEnabled = enable
        findViewById<RadioButton>(R.id.PowerSavingButton).isEnabled = enable

        findViewById<RadioGroup>(R.id.LatencySelectionGroup).check(
            if (performanceMode == 0) R.id.LowLatencyButton
            else if (performanceMode == 1) R.id.NormalLatencyButton
            else R.id.PowerSavingButton
        )
    }

    private fun EnableAsyncModeUI(enable: Boolean) {
        findViewById<CheckBox>(R.id.asyncProcessingCheckbox).isEnabled = enable
        findViewById<CheckBox>(R.id.asyncProcessingCheckbox).isChecked = isAsyncMode
        if (isAsyncMode) {
            performanceMode = 0
            findViewById<RadioGroup>(R.id.LatencySelectionGroup).check(
                R.id.LowLatencyButton
            )
            EnablePerformanceModeUI(false)
        }
    }

    override fun onDestroy() {
        onStopTest()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val serviceIntent = Intent(BeatriceForegroundService.ACTION_STOP, null, this, BeatriceForegroundService::class.java)
            startForegroundService(serviceIntent)
        }
        super.onDestroy()
    }

    fun deleteDirectoryContents(dir: File): Boolean {
        if (!dir.exists() || !dir.isDirectory) return false

        var success = true
        dir.listFiles()?.forEach { file ->
            success = success && deleteRecursively(file)
        }
        return success
    }
    fun deleteRecursively(file: File): Boolean {
        return if (file.isDirectory) {
            file.listFiles()?.forEach { child ->
                if (!deleteRecursively(child)) return false
            }
            file.delete()
        } else {
            file.delete()
        }
    }
    fun copyFilteredFilesRecursively(
        source: DocumentFile,
        destRoot: File,
        relativePath: String
    ) {
        for (file in source.listFiles()) {
            val newRelativePath = if (relativePath.isEmpty()) file.name ?: "" else "$relativePath/${file.name}"
            if (file.isDirectory) {
                copyFilteredFilesRecursively(file, destRoot, newRelativePath)
            } else if (file.isFile && isTargetExtension(file.name)) {
                val destFile = File(destRoot, newRelativePath)
                destFile.parentFile?.mkdirs()
                copyFile(file, destFile)
            }
        }
    }
    fun copyFile(source: DocumentFile, dest: File) {
        try {
            contentResolver.openInputStream(source.uri)?.use { input ->
                FileOutputStream(dest).use { output ->
                    input.copyTo(output)
                }
            }
        } catch (e: Exception) {
            Log.e("CopyFile", "Failed to copy ${source.name}", e)
        }
    }
    fun isTargetExtension(name: String): Boolean {
        return name.endsWith(".toml", ignoreCase = true) ||
            name.endsWith(".bin", ignoreCase = true) ||
            name.endsWith(".png", ignoreCase = true)
    }

    private fun copyModelFilesToExtDir(tomlUri: Uri) {
        val tomlFile = DocumentFile.fromSingleUri(this, tomlUri) ?: return

        val docId = DocumentsContract.getDocumentId(tomlUri)
        val parentDocId = docId.substringBeforeLast("/")
        val parentUri = DocumentsContract.buildTreeDocumentUri(
            tomlUri.authority ?: return,
            parentDocId
        )
        val parentDir = requireNotNull( DocumentFile.fromTreeUri(this, parentUri) )

        val destRoot = getExternalFilesDir(null) ?: filesDir
        deleteDirectoryContents(destRoot)
        copyFilteredFilesRecursively(parentDir, destRoot, "")
        beatriceEngine.readModel(tomlFile.name?.let { destRoot.resolve(it).absolutePath } ?: return)
        updateModelInfo()
    }


    private fun onStartTest() {
        beatriceEngine.create(
            getAssets(),
            this.getExternalFilesDir(null)?.absolutePath ?: this.filesDir.absolutePath
        )
        mAAudioRecommended = beatriceEngine.isAAudioRecommended()
        EnableAudioApiUI(true)
        EnablePerformanceModeUI(true)
        EnableAsyncModeUI(true)
        
        beatriceEngine.setAPI(apiSelection)
        beatriceEngine.setPerformanceMode(performanceMode)
        beatriceEngine.setAsyncMode(isAsyncMode)
        updateModelInfo()
    }

    private fun updateModelInfo(){

        val modelName = beatriceEngine.getModelName()
        val modelLabel = findViewById<TextView>(R.id.ModelName)
        modelLabel.text = if (modelName.isNotEmpty()) modelName else getString(R.string.model_name)

        val voiceNameList = ArrayList<String>()
        for(i in 0 until 256) {
            val voiceName = beatriceEngine.getVoiceName(i)
            if (voiceName.isNotEmpty()) {
                voiceNameList.add(voiceName)
            }else{
                break
            }
        }
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, voiceNameList)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        voiceSpinner.adapter = adapter
        beatriceEngine.setVoiceID(0)
    }

    private fun onStopTest() {
        stopEffect()
        beatriceEngine.delete()
    }

    private fun toggleEffect() {
        if (isPlaying) {
            stopEffect()
        } else {
            beatriceEngine.setAPI(apiSelection)
            beatriceEngine.setPerformanceMode(performanceMode)
            beatriceEngine.setAsyncMode(isAsyncMode)
            startEffect()
        }
    }

    private fun startEffect() {
        Log.d(TAG, "Attempting to start")
        val success = beatriceEngine.setEffectOn(true)
        if (success) {
            statusText.setText(R.string.status_playing)
            toggleEffectButton.setText(R.string.stop_effect)
            isPlaying = true
            EnableAudioApiUI(false)
            EnablePerformanceModeUI(false)
            EnableAsyncModeUI(false)
        } else {
            statusText.setText(R.string.status_open_failed)
            isPlaying = false
        }
    }

    private fun stopEffect() {
        Log.d(TAG, "Playing, attempting to stop")
        beatriceEngine.setEffectOn(false)
        resetStatusView()
        toggleEffectButton.setText(R.string.start_effect)
        isPlaying = false
        EnableAudioApiUI(true)

        val enabled = findViewById<RadioButton>(R.id.aaudioButton).isChecked
        EnablePerformanceModeUI(enabled)
        EnableAsyncModeUI(enabled)
    }

    private fun setSpinnersEnabled(isEnabled: Boolean) {
        val enabled = if (findViewById<RadioButton>(R.id.slesButton).isChecked) {
            playbackDeviceSpinner.setSelection(0)
            recordingDeviceSpinner.setSelection(0)
            false
        } else {
            isEnabled
        }
        recordingDeviceSpinner.isEnabled = enabled
        playbackDeviceSpinner.isEnabled = enabled
    }

    private fun getRecordingDeviceId(): Int {
        return (recordingDeviceSpinner.selectedItem as AudioDeviceListEntry).id
    }

    private fun getPlaybackDeviceId(): Int {
        return (playbackDeviceSpinner.selectedItem as AudioDeviceListEntry).id
    }

    private fun isRecordPermissionGranted(): Boolean {
        return ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
    }

    private fun requestRecordPermission() {
        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), AUDIO_EFFECT_REQUEST)
    }

    private fun resetStatusView() {
        statusText.setText(R.string.status_warning)
    }

    private fun startForegroundService() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val serviceIntent = Intent(BeatriceForegroundService.ACTION_START, null, this, BeatriceForegroundService::class.java)
            startForegroundService(serviceIntent)
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        if (requestCode != AUDIO_EFFECT_REQUEST) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults)
            return
        }

        if (grantResults.size != 1 || grantResults[0] != PackageManager.PERMISSION_GRANTED) {
            statusText.setText(R.string.status_record_audio_denied)
            Toast.makeText(applicationContext, getString(R.string.need_record_audio_permission), Toast.LENGTH_SHORT).show()
            EnableAudioApiUI(false)
            toggleEffectButton.isEnabled = false
        } else {
            startForegroundService()
        }
    }

    protected override fun onPause() {
        super.onPause()
        if (isPlaying) {
            stopEffect()
        }
    }

}