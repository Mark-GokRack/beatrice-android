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

class AdvancedFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_advanced, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

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
