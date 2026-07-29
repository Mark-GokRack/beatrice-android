package com.gokrack.beatriceapp

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.RadioGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import kotlin.math.roundToInt
import androidx.lifecycle.ViewModelProvider
import com.google.android.material.slider.RangeSlider
import com.google.android.material.slider.Slider

class ParamsFragment : Fragment() {

    private lateinit var viewModel: EngineStateViewModel
    private var isRestoring = false

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_params, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        viewModel = ViewModelProvider(requireActivity())[EngineStateViewModel::class.java]

        bindControls(view)

        viewModel.settingsResetTrigger.observe(viewLifecycleOwner) {
            isRestoring = true
            applyCurrentSettings(view)
            isRestoring = false
        }

        isRestoring = true
        applyCurrentSettings(view)
        isRestoring = false
    }

    private fun bindControls(view: View) {
        val inputGainValue = view.findViewById<TextView>(R.id.input_gain_value)
        val inputGainSlider = view.findViewById<Slider>(R.id.input_gain_slider)
        view.findViewById<Button>(R.id.input_gain_decrement).setOnClickListener {
            inputGainSlider.value = (inputGainSlider.value - inputGainSlider.stepSize)
                .coerceAtLeast(inputGainSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.input_gain_increment).setOnClickListener {
            inputGainSlider.value = (inputGainSlider.value + inputGainSlider.stepSize)
                .coerceAtMost(inputGainSlider.valueTo)
        }
        inputGainSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            inputGainValue.text = "%.1f dB".format(value)
            SettingsManager.saveInputGain(value)
            beatriceEngine.setInputGain(value.toDouble())
        }

        val outputGainValue = view.findViewById<TextView>(R.id.output_gain_value)
        val outputGainSlider = view.findViewById<Slider>(R.id.output_gain_slider)
        view.findViewById<Button>(R.id.output_gain_decrement).setOnClickListener {
            outputGainSlider.value = (outputGainSlider.value - outputGainSlider.stepSize)
                .coerceAtLeast(outputGainSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.output_gain_increment).setOnClickListener {
            outputGainSlider.value = (outputGainSlider.value + outputGainSlider.stepSize)
                .coerceAtMost(outputGainSlider.valueTo)
        }
        outputGainSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            outputGainValue.text = "%.1f dB".format(value)
            SettingsManager.saveOutputGain(value)
            beatriceEngine.setOutputGain(value.toDouble())
        }

        val pitchShiftValue = view.findViewById<TextView>(R.id.pitch_shift_value)
        val pitchShiftSlider = view.findViewById<Slider>(R.id.pitch_shift_slider)
        view.findViewById<Button>(R.id.pitch_shift_decrement).setOnClickListener {
            pitchShiftSlider.value = (pitchShiftSlider.value - pitchShiftSlider.stepSize)
                .coerceAtLeast(pitchShiftSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.pitch_shift_increment).setOnClickListener {
            pitchShiftSlider.value = (pitchShiftSlider.value + pitchShiftSlider.stepSize)
                .coerceAtMost(pitchShiftSlider.valueTo)
        }
        pitchShiftSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            pitchShiftValue.text = "%.2f".format(value)
            SettingsManager.savePitchShift(value)
            beatriceEngine.setPitchShift(value.toDouble())
        }

        val formantShiftValue = view.findViewById<TextView>(R.id.formant_shift_value)
        val formantShiftSlider = view.findViewById<Slider>(R.id.formant_shift_slider)
        view.findViewById<Button>(R.id.formant_shift_decrement).setOnClickListener {
            formantShiftSlider.value = (formantShiftSlider.value - formantShiftSlider.stepSize)
                .coerceAtLeast(formantShiftSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.formant_shift_increment).setOnClickListener {
            formantShiftSlider.value = (formantShiftSlider.value + formantShiftSlider.stepSize)
                .coerceAtMost(formantShiftSlider.valueTo)
        }
        formantShiftSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            formantShiftValue.text = "%.1f".format(value)
            SettingsManager.saveFormantShift(value)
            beatriceEngine.setFormantShift(value.toDouble())
        }

        val vqValue = view.findViewById<TextView>(R.id.vq_neighbors_value)
        val vqSlider = view.findViewById<Slider>(R.id.vq_neighbors_slider)
        view.findViewById<Button>(R.id.vq_neighbors_decrement).setOnClickListener {
            vqSlider.value = (vqSlider.value - vqSlider.stepSize)
                .coerceAtLeast(vqSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.vq_neighbors_increment).setOnClickListener {
            vqSlider.value = (vqSlider.value + vqSlider.stepSize)
                .coerceAtMost(vqSlider.valueTo)
        }
        vqSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            val intValue = value.toInt()
            vqValue.text = intValue.toString()
            SettingsManager.saveVQNeighbors(intValue)
            beatriceEngine.setVQNumNeighbors(intValue)
        }

        val advancedContent = view.findViewById<View>(R.id.advanced_section_content)
        val advancedArrow = view.findViewById<TextView>(R.id.advanced_section_arrow)
        view.findViewById<View>(R.id.advanced_section_header).setOnClickListener {
            if (advancedContent.visibility == View.VISIBLE) {
                advancedContent.visibility = View.GONE
                advancedArrow.text = "\u25BC"
            } else {
                advancedContent.visibility = View.VISIBLE
                advancedArrow.text = "\u25B2"
            }
        }

        val intonationValue = view.findViewById<TextView>(R.id.intonation_intensity_value)
        val intonationSlider = view.findViewById<Slider>(R.id.intonation_intensity_slider)
        view.findViewById<Button>(R.id.intonation_intensity_decrement).setOnClickListener {
            intonationSlider.value = (intonationSlider.value - intonationSlider.stepSize)
                .coerceAtLeast(intonationSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.intonation_intensity_increment).setOnClickListener {
            intonationSlider.value = (intonationSlider.value + intonationSlider.stepSize)
                .coerceAtMost(intonationSlider.valueTo)
        }
        intonationSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            intonationValue.text = "%.2f".format(value)
            SettingsManager.saveIntonationIntensity(value)
            beatriceEngine.setIntonationIntensity(value.toDouble())
        }

        val pitchCorrectionValue = view.findViewById<TextView>(R.id.pitch_correction_value)
        val pitchCorrectionSlider = view.findViewById<Slider>(R.id.pitch_correction_slider)
        view.findViewById<Button>(R.id.pitch_correction_decrement).setOnClickListener {
            pitchCorrectionSlider.value = (pitchCorrectionSlider.value - pitchCorrectionSlider.stepSize)
                .coerceAtLeast(pitchCorrectionSlider.valueFrom)
        }
        view.findViewById<Button>(R.id.pitch_correction_increment).setOnClickListener {
            pitchCorrectionSlider.value = (pitchCorrectionSlider.value + pitchCorrectionSlider.stepSize)
                .coerceAtMost(pitchCorrectionSlider.valueTo)
        }
        pitchCorrectionSlider.addOnChangeListener { _, value, _ ->
            if (isRestoring) return@addOnChangeListener
            pitchCorrectionValue.text = "%.2f".format(value)
            SettingsManager.savePitchCorrection(value)
            beatriceEngine.setPitchCorrection(value.toDouble())
        }

        view.findViewById<RadioGroup>(R.id.pitch_correction_mode_group)
            .setOnCheckedChangeListener { _, checkedId ->
                if (isRestoring) return@setOnCheckedChangeListener
                val mode = when (checkedId) {
                    R.id.pcm_hard0 -> 0
                    R.id.pcm_hard1 -> 1
                    else -> 0
                }
                SettingsManager.savePitchCorrectionMode(mode)
                beatriceEngine.setPitchCorrectionMode(mode)
            }

        val rangeText = view.findViewById<TextView>(R.id.source_pitch_range_value)
        val rangeSlider = view.findViewById<RangeSlider>(R.id.source_pitch_range_slider)
        rangeSlider.addOnChangeListener { slider, _, _ ->
            if (isRestoring) return@addOnChangeListener
            val values = slider.values
            val valuesInHz = listOf((440 * Math.pow(2.0, (values[0] - 69) / 12.0)).roundToInt(), (440 * Math.pow(2.0, (values[1] - 69) / 12.0)).roundToInt())
            rangeText.text = "${values[0]} - ${values[1]} (${valuesInHz[0]} Hz - ${valuesInHz[1]} Hz)"
            SettingsManager.saveSourcePitchRange(values[0], values[1])
            beatriceEngine.setSourcePitchRange(values[0].toDouble(), values[1].toDouble())
        }
    }

    private fun applyCurrentSettings(view: View) {
        val inputGainValue = view.findViewById<TextView>(R.id.input_gain_value)
        val inputGainSlider = view.findViewById<Slider>(R.id.input_gain_slider)
        inputGainSlider.value = SettingsManager.loadInputGain()
        inputGainValue.text = "%.1f dB".format(inputGainSlider.value)
        SettingsManager.saveInputGain(inputGainSlider.value)
        beatriceEngine.setInputGain(inputGainSlider.value.toDouble())

        val outputGainValue = view.findViewById<TextView>(R.id.output_gain_value)
        val outputGainSlider = view.findViewById<Slider>(R.id.output_gain_slider)
        outputGainSlider.value = SettingsManager.loadOutputGain()
        outputGainValue.text = "%.1f dB".format(outputGainSlider.value)
        SettingsManager.saveOutputGain(outputGainSlider.value)
        beatriceEngine.setOutputGain(outputGainSlider.value.toDouble())

        val pitchShiftValue = view.findViewById<TextView>(R.id.pitch_shift_value)
        val pitchShiftSlider = view.findViewById<Slider>(R.id.pitch_shift_slider)
        pitchShiftSlider.value = SettingsManager.loadPitchShift()
        pitchShiftValue.text = "%.2f".format(pitchShiftSlider.value)
        SettingsManager.savePitchShift(pitchShiftSlider.value)
        beatriceEngine.setPitchShift(pitchShiftSlider.value.toDouble())

        val formantShiftValue = view.findViewById<TextView>(R.id.formant_shift_value)
        val formantShiftSlider = view.findViewById<Slider>(R.id.formant_shift_slider)
        formantShiftSlider.value = SettingsManager.loadFormantShift()
        formantShiftValue.text = "%.1f".format(formantShiftSlider.value)
        SettingsManager.saveFormantShift(formantShiftSlider.value)
        beatriceEngine.setFormantShift(formantShiftSlider.value.toDouble())

        val vqValue = view.findViewById<TextView>(R.id.vq_neighbors_value)
        val vqSlider = view.findViewById<Slider>(R.id.vq_neighbors_slider)
        vqSlider.value = SettingsManager.loadVQNeighbors().toFloat()
        vqValue.text = vqSlider.value.toInt().toString()
        SettingsManager.saveVQNeighbors(vqSlider.value.toInt())
        beatriceEngine.setVQNumNeighbors(vqSlider.value.toInt())

        val intonationValue = view.findViewById<TextView>(R.id.intonation_intensity_value)
        val intonationSlider = view.findViewById<Slider>(R.id.intonation_intensity_slider)
        intonationSlider.value = SettingsManager.loadIntonationIntensity()
        intonationValue.text = "%.2f".format(intonationSlider.value)
        SettingsManager.saveIntonationIntensity(intonationSlider.value)
        beatriceEngine.setIntonationIntensity(intonationSlider.value.toDouble())

        val pitchCorrectionValue = view.findViewById<TextView>(R.id.pitch_correction_value)
        val pitchCorrectionSlider = view.findViewById<Slider>(R.id.pitch_correction_slider)
        pitchCorrectionSlider.value = SettingsManager.loadPitchCorrection()
        pitchCorrectionValue.text = "%.2f".format(pitchCorrectionSlider.value)
        SettingsManager.savePitchCorrection(pitchCorrectionSlider.value)
        beatriceEngine.setPitchCorrection(pitchCorrectionSlider.value.toDouble())

        val pitchCorrectionMode = SettingsManager.loadPitchCorrectionMode()
        view.findViewById<RadioGroup>(R.id.pitch_correction_mode_group).check(
            if (pitchCorrectionMode == 1) R.id.pcm_hard1 else R.id.pcm_hard0
        )
        beatriceEngine.setPitchCorrectionMode(pitchCorrectionMode)

        val rangeText = view.findViewById<TextView>(R.id.source_pitch_range_value)
        val rangeSlider = view.findViewById<RangeSlider>(R.id.source_pitch_range_slider)
        val minPitch = SettingsManager.loadSourcePitchRangeMin()
        val maxPitch = SettingsManager.loadSourcePitchRangeMax()
        val minHz = (440 * Math.pow(2.0, (minPitch - 69) / 12.0)).roundToInt()
        val maxHz = (440 * Math.pow(2.0, (maxPitch - 69) / 12.0)).roundToInt()
        rangeText.text = "$minPitch - $maxPitch ($minHz Hz - $maxHz Hz)"
        rangeSlider.values = listOf(minPitch, maxPitch)
        SettingsManager.saveSourcePitchRange(minPitch, maxPitch)
        beatriceEngine.setSourcePitchRange(minPitch.toDouble(), maxPitch.toDouble())
    }
}
