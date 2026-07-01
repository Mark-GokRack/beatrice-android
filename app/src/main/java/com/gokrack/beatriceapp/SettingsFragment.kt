package com.gokrack.beatriceapp

import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.CheckBox
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.google.oboe.samples.audio_device.AudioDeviceListEntry
import com.google.oboe.samples.audio_device.AudioDeviceSpinner

class SettingsFragment : Fragment() {

    private lateinit var viewModel: EngineStateViewModel
    private lateinit var recordingDeviceSpinner: AudioDeviceSpinner
    private lateinit var playbackDeviceSpinner: AudioDeviceSpinner

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_settings, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        viewModel = ViewModelProvider(requireActivity())[EngineStateViewModel::class.java]

        // Device spinners
        recordingDeviceSpinner = view.findViewById(R.id.recording_devices_spinner)
        playbackDeviceSpinner = view.findViewById(R.id.playback_devices_spinner)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            recordingDeviceSpinner.setDirectionType(AudioManager.GET_DEVICES_INPUTS)
            recordingDeviceSpinner.onItemSelectedListener =
                object : AdapterView.OnItemSelectedListener {
                    override fun onItemSelected(
                        parent: AdapterView<*>, v: View?, position: Int, id: Long
                    ) {
                        beatriceEngine.setRecordingDeviceId(
                            (recordingDeviceSpinner.selectedItem as AudioDeviceListEntry).id
                        )
                    }
                    override fun onNothingSelected(parent: AdapterView<*>) {}
                }

            playbackDeviceSpinner.setDirectionType(AudioManager.GET_DEVICES_OUTPUTS)
            playbackDeviceSpinner.onItemSelectedListener =
                object : AdapterView.OnItemSelectedListener {
                    override fun onItemSelected(
                        parent: AdapterView<*>, v: View?, position: Int, id: Long
                    ) {
                        beatriceEngine.setPlaybackDeviceId(
                            (playbackDeviceSpinner.selectedItem as AudioDeviceListEntry).id
                        )
                    }
                    override fun onNothingSelected(parent: AdapterView<*>) {}
                }
        }

        // Audio API selection
        val apiGroup = view.findViewById<RadioGroup>(R.id.apiSelectionGroup)
        val aaudioBtn = view.findViewById<RadioButton>(R.id.aaudioButton)
        val slesBtn = view.findViewById<RadioButton>(R.id.slesButton)
        apiGroup.check(if ((viewModel.apiSelection.value ?: 0) == 0) R.id.aaudioButton else R.id.slesButton)

        aaudioBtn.setOnClickListener {
            viewModel.apiSelection.value = 0
            updateLatencyAndAsync(enabled = true, asyncEnabled = true)
            setSpinnersEnabled(true)
        }
        slesBtn.setOnClickListener {
            viewModel.apiSelection.value = 1
            updateLatencyAndAsync(enabled = false, asyncEnabled = false)
            setSpinnersEnabled(false)
        }

        // Latency mode
        val latencyGroup = view.findViewById<RadioGroup>(R.id.LatencySelectionGroup)
        latencyGroup.check(
            when (viewModel.performanceMode.value ?: 0) {
                1 -> R.id.NormalLatencyButton
                2 -> R.id.PowerSavingButton
                else -> R.id.LowLatencyButton
            }
        )
        view.findViewById<RadioButton>(R.id.LowLatencyButton).setOnClickListener {
            viewModel.performanceMode.value = 0
        }
        view.findViewById<RadioButton>(R.id.NormalLatencyButton).setOnClickListener {
            viewModel.performanceMode.value = 1
        }
        view.findViewById<RadioButton>(R.id.PowerSavingButton).setOnClickListener {
            viewModel.performanceMode.value = 2
        }

        // Async processing
        val asyncCheckbox = view.findViewById<CheckBox>(R.id.asyncProcessingCheckbox)
        asyncCheckbox.isChecked = viewModel.isAsyncMode.value ?: true
        asyncCheckbox.setOnCheckedChangeListener { _, isChecked ->
            viewModel.isAsyncMode.value = isChecked
            if (isChecked) {
                viewModel.performanceMode.value = 0
                latencyGroup.check(R.id.LowLatencyButton)
                setLatencyEnabled(false)
            } else {
                setLatencyEnabled(true)
            }
        }

        // Observe running state — disable settings while engine is active
        viewModel.isEngineRunning.observe(viewLifecycleOwner) { running ->
            setAllSettingsEnabled(!running)
        }

        viewModel.isAAudioRecommended.observe(viewLifecycleOwner) { recommended ->
            aaudioBtn.isEnabled = recommended && (viewModel.isEngineRunning.value != true)
        }

        // Status text at the bottom of the System tab
        val statusView = view.findViewById<TextView>(R.id.status_view_text)
        viewModel.statusText.observe(viewLifecycleOwner) { text ->
            if (text.isNotEmpty()) statusView.text = text
        }
    }

    private fun setAllSettingsEnabled(enabled: Boolean) {
        val view = view ?: return
        val isAAudio = (viewModel.apiSelection.value ?: 0) == 0
        val aaudioBtn = view.findViewById<RadioButton>(R.id.aaudioButton)
        val slesBtn = view.findViewById<RadioButton>(R.id.slesButton)

        aaudioBtn.isEnabled = enabled && (viewModel.isAAudioRecommended.value == true)
        slesBtn.isEnabled = enabled
        view.findViewById<RadioGroup>(R.id.apiSelectionGroup).check(
            if (isAAudio) R.id.aaudioButton else R.id.slesButton
        )

        updateLatencyAndAsync(
            enabled = enabled && isAAudio,
            asyncEnabled = enabled && isAAudio
        )
        setSpinnersEnabled(enabled && isAAudio)
    }

    private fun updateLatencyAndAsync(enabled: Boolean, asyncEnabled: Boolean) {
        setLatencyEnabled(enabled && !(viewModel.isAsyncMode.value == true && enabled))
        view?.findViewById<CheckBox>(R.id.asyncProcessingCheckbox)?.isEnabled = asyncEnabled
    }

    private fun setLatencyEnabled(enabled: Boolean) {
        val v = view ?: return
        v.findViewById<RadioButton>(R.id.LowLatencyButton).isEnabled = enabled
        v.findViewById<RadioButton>(R.id.NormalLatencyButton).isEnabled = enabled
        v.findViewById<RadioButton>(R.id.PowerSavingButton).isEnabled = enabled
    }

    private fun setSpinnersEnabled(isEnabled: Boolean) {
        val enabled = if ((viewModel.apiSelection.value ?: 0) == 1) {
            playbackDeviceSpinner.setSelection(0)
            recordingDeviceSpinner.setSelection(0)
            false
        } else {
            isEnabled
        }
        recordingDeviceSpinner.isEnabled = enabled
        playbackDeviceSpinner.isEnabled = enabled
    }
}
