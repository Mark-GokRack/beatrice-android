package com.gokrack.beatriceapp

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.RadioGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.google.android.material.slider.RangeSlider
import com.google.android.material.slider.Slider

class BasicFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_basic, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // InputGain
        val inputGainValue = view.findViewById<TextView>(R.id.input_gain_value)
        val inputGainSlider = view.findViewById<Slider>(R.id.input_gain_slider)
        inputGainSlider.addOnChangeListener { _, value, _ ->
            inputGainValue.text = "%.1f dB".format(value)
            beatriceEngine.setInputGain(value.toDouble())
        }
        inputGainValue.text = "%.1f dB".format(inputGainSlider.value)
        view.findViewById<android.widget.Button>(R.id.input_gain_decrement).setOnClickListener {
            val newValue = (inputGainSlider.value - inputGainSlider.stepSize)
                .coerceAtLeast(inputGainSlider.valueFrom)
            inputGainSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.input_gain_increment).setOnClickListener {
            val newValue = (inputGainSlider.value + inputGainSlider.stepSize)
                .coerceAtMost(inputGainSlider.valueTo)
            inputGainSlider.value = newValue
        }

        // OutputGain
        val outputGainValue = view.findViewById<TextView>(R.id.output_gain_value)
        val outputGainSlider = view.findViewById<Slider>(R.id.output_gain_slider)
        outputGainSlider.addOnChangeListener { _, value, _ ->
            outputGainValue.text = "%.1f dB".format(value)
            beatriceEngine.setOutputGain(value.toDouble())
        }
        outputGainValue.text = "%.1f dB".format(outputGainSlider.value)
        view.findViewById<android.widget.Button>(R.id.output_gain_decrement).setOnClickListener {
            val newValue = (outputGainSlider.value - outputGainSlider.stepSize)
                .coerceAtLeast(outputGainSlider.valueFrom)
            outputGainSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.output_gain_increment).setOnClickListener {
            val newValue = (outputGainSlider.value + outputGainSlider.stepSize)
                .coerceAtMost(outputGainSlider.valueTo)
            outputGainSlider.value = newValue
        }

        // PitchShift
        val pitchShiftValue = view.findViewById<TextView>(R.id.pitch_shift_value)
        val pitchShiftSlider = view.findViewById<Slider>(R.id.pitch_shift_slider)
        pitchShiftSlider.addOnChangeListener { _, value, _ ->
            pitchShiftValue.text = "%.2f".format(value)
            beatriceEngine.setPitchShift(value.toDouble())
        }
        pitchShiftValue.text = "%.2f".format(pitchShiftSlider.value)
        view.findViewById<android.widget.Button>(R.id.pitch_shift_decrement).setOnClickListener {
            val newValue = (pitchShiftSlider.value - pitchShiftSlider.stepSize)
                .coerceAtLeast(pitchShiftSlider.valueFrom)
            pitchShiftSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.pitch_shift_increment).setOnClickListener {
            val newValue = (pitchShiftSlider.value + pitchShiftSlider.stepSize)
                .coerceAtMost(pitchShiftSlider.valueTo)
            pitchShiftSlider.value = newValue
        }

        // FormantShift
        val formantShiftValue = view.findViewById<TextView>(R.id.formant_shift_value)
        val formantShiftSlider = view.findViewById<Slider>(R.id.formant_shift_slider)
        formantShiftSlider.addOnChangeListener { _, value, _ ->
            formantShiftValue.text = "%.1f".format(value)
            beatriceEngine.setFormantShift(value.toDouble())
        }
        formantShiftValue.text = "%.1f".format(formantShiftSlider.value)
        view.findViewById<android.widget.Button>(R.id.formant_shift_decrement).setOnClickListener {
            val newValue = (formantShiftSlider.value - formantShiftSlider.stepSize)
                .coerceAtLeast(formantShiftSlider.valueFrom)
            formantShiftSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.formant_shift_increment).setOnClickListener {
            val newValue = (formantShiftSlider.value + formantShiftSlider.stepSize)
                .coerceAtMost(formantShiftSlider.valueTo)
            formantShiftSlider.value = newValue
        }

        // VQ Neighbors
        val vqValue = view.findViewById<TextView>(R.id.vq_neighbors_value)
        val vqSlider = view.findViewById<Slider>(R.id.vq_neighbors_slider)
        vqSlider.addOnChangeListener { _, value, _ ->
            val intValue = value.toInt()
            vqValue.text = intValue.toString()
            beatriceEngine.setVQNumNeighbors(intValue)
        }
        vqValue.text = vqSlider.value.toInt().toString()
        view.findViewById<android.widget.Button>(R.id.vq_neighbors_decrement).setOnClickListener {
            val newValue = (vqSlider.value - vqSlider.stepSize)
                .coerceAtLeast(vqSlider.valueFrom)
            vqSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.vq_neighbors_increment).setOnClickListener {
            val newValue = (vqSlider.value + vqSlider.stepSize)
                .coerceAtMost(vqSlider.valueTo)
            vqSlider.value = newValue
        }

        // Advanced section toggle
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

        // IntonationIntensity
        val intonationValue = view.findViewById<TextView>(R.id.intonation_intensity_value)
        val intonationSlider = view.findViewById<Slider>(R.id.intonation_intensity_slider)
        intonationSlider.addOnChangeListener { _, value, _ ->
            intonationValue.text = "%.2f".format(value)
            beatriceEngine.setIntonationIntensity(value.toDouble())
        }
        intonationValue.text = "%.2f".format(intonationSlider.value)
        view.findViewById<android.widget.Button>(R.id.intonation_intensity_decrement).setOnClickListener {
            val newValue = (intonationSlider.value - intonationSlider.stepSize)
                .coerceAtLeast(intonationSlider.valueFrom)
            intonationSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.intonation_intensity_increment).setOnClickListener {
            val newValue = (intonationSlider.value + intonationSlider.stepSize)
                .coerceAtMost(intonationSlider.valueTo)
            intonationSlider.value = newValue
        }

        // PitchCorrection
        val pitchCorrectionValue = view.findViewById<TextView>(R.id.pitch_correction_value)
        val pitchCorrectionSlider = view.findViewById<Slider>(R.id.pitch_correction_slider)
        pitchCorrectionSlider.addOnChangeListener { _, value, _ ->
            pitchCorrectionValue.text = "%.2f".format(value)
            beatriceEngine.setPitchCorrection(value.toDouble())
        }
        pitchCorrectionValue.text = "%.2f".format(pitchCorrectionSlider.value)
        view.findViewById<android.widget.Button>(R.id.pitch_correction_decrement).setOnClickListener {
            val newValue = (pitchCorrectionSlider.value - pitchCorrectionSlider.stepSize)
                .coerceAtLeast(pitchCorrectionSlider.valueFrom)
            pitchCorrectionSlider.value = newValue
        }
        view.findViewById<android.widget.Button>(R.id.pitch_correction_increment).setOnClickListener {
            val newValue = (pitchCorrectionSlider.value + pitchCorrectionSlider.stepSize)
                .coerceAtMost(pitchCorrectionSlider.valueTo)
            pitchCorrectionSlider.value = newValue
        }

        // PitchCorrectionMode
        view.findViewById<RadioGroup>(R.id.pitch_correction_mode_group)
            .setOnCheckedChangeListener { _, checkedId ->
                val mode = when (checkedId) {
                    R.id.pcm_hard0 -> 0
                    R.id.pcm_hard1 -> 1
                    else -> 0
                }
                beatriceEngine.setPitchCorrectionMode(mode)
            }

        // SourcePitchRange
        val rangeText = view.findViewById<TextView>(R.id.source_pitch_range_value)
        val rangeSlider = view.findViewById<RangeSlider>(R.id.source_pitch_range_slider)
        rangeSlider.addOnChangeListener { slider, _, _ ->
            val vals = slider.values
            rangeText.text = "${vals[0]} - ${vals[1]}"
            beatriceEngine.setSourcePitchRange(vals[0].toDouble(), vals[1].toDouble())
        }
        val initVals = rangeSlider.values
        rangeText.text = "${initVals[0]} - ${initVals[1]}"
    }
}
