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

class PitchFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_pitch, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // PitchShift
        val pitchShiftValue = view.findViewById<TextView>(R.id.pitch_shift_value)
        val pitchShiftSlider = view.findViewById<Slider>(R.id.pitch_shift_slider)
        pitchShiftSlider.addOnChangeListener { _, value, _ ->
            pitchShiftValue.text = "%.2f".format(value)
            beatriceEngine.setPitchShift(value.toDouble())
        }
        pitchShiftValue.text = "%.2f".format(pitchShiftSlider.value)

        // FormantShift
        val formantShiftValue = view.findViewById<TextView>(R.id.formant_shift_value)
        val formantShiftSlider = view.findViewById<Slider>(R.id.formant_shift_slider)
        formantShiftSlider.addOnChangeListener { _, value, _ ->
            formantShiftValue.text = "%.1f".format(value)
            beatriceEngine.setFormantShift(value.toDouble())
        }
        formantShiftValue.text = "%.1f".format(formantShiftSlider.value)

        // IntonationIntensity
        val intonationValue = view.findViewById<TextView>(R.id.intonation_intensity_value)
        val intonationSlider = view.findViewById<Slider>(R.id.intonation_intensity_slider)
        intonationSlider.addOnChangeListener { _, value, _ ->
            intonationValue.text = "%.2f".format(value)
            beatriceEngine.setIntonationIntensity(value.toDouble())
        }
        intonationValue.text = "%.2f".format(intonationSlider.value)

        // PitchCorrection
        val pitchCorrectionValue = view.findViewById<TextView>(R.id.pitch_correction_value)
        val pitchCorrectionSlider = view.findViewById<Slider>(R.id.pitch_correction_slider)
        pitchCorrectionSlider.addOnChangeListener { _, value, _ ->
            pitchCorrectionValue.text = "%.2f".format(value)
            beatriceEngine.setPitchCorrection(value.toDouble())
        }
        pitchCorrectionValue.text = "%.2f".format(pitchCorrectionSlider.value)

        // PitchCorrectionMode
        view.findViewById<RadioGroup>(R.id.pitch_correction_mode_group)
            .setOnCheckedChangeListener { _, checkedId ->
                val mode = when (checkedId) {
                    R.id.pcm_off -> 0
                    R.id.pcm_chromatic -> 1
                    R.id.pcm_scale -> 2
                    else -> 0
                }
                beatriceEngine.setPitchCorrectionMode(mode)
            }

        // SourcePitchRange
        val rangeText = view.findViewById<TextView>(R.id.source_pitch_range_value)
        val rangeSlider = view.findViewById<RangeSlider>(R.id.source_pitch_range_slider)
        rangeSlider.addOnChangeListener { slider, _, _ ->
            val vals = slider.values
            rangeText.text = "${vals[0].toInt()} - ${vals[1].toInt()} Hz"
            beatriceEngine.setSourcePitchRange(vals[0].toDouble(), vals[1].toDouble())
        }
        val initVals = rangeSlider.values
        rangeText.text = "${initVals[0].toInt()} - ${initVals[1].toInt()} Hz"
    }
}
