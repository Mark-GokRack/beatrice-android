package com.gokrack.beatriceAndroid

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

class MainActivity : Activity(), ActivityCompat.OnRequestPermissionsResultCallback {

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

    private var isPlaying = false
    private var apiSelection = OBOE_API_AAUDIO
    private var mAAudioRecommended = true

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

        findViewById<RadioGroup>(R.id.apiSelectionGroup).check(R.id.aaudioButton)
        findViewById<RadioButton>(R.id.aaudioButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                apiSelection = OBOE_API_AAUDIO
                setSpinnersEnabled(true)
            }
        }
        findViewById<RadioButton>(R.id.slesButton).setOnClickListener {
            if ((it as RadioButton).isChecked) {
                apiSelection = OBOE_API_OPENSL_ES
                setSpinnersEnabled(false)
            }
        }

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

    override fun onDestroy() {
        onStopTest()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            val serviceIntent = Intent(BeatriceForegroundService.ACTION_STOP, null, this, BeatriceForegroundService::class.java)
            startForegroundService(serviceIntent)
        }
        super.onDestroy()
    }

    private fun onStartTest() {
        
        beatriceEngine.create(
            getAssets(),
            this.filesDir.absolutePath
        )
        mAAudioRecommended = beatriceEngine.isAAudioRecommended()
        EnableAudioApiUI(true)
        beatriceEngine.setAPI(apiSelection)
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
}