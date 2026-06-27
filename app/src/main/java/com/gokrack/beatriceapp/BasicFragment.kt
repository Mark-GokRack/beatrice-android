package com.gokrack.beatriceapp

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
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

        // OutputGain
        val outputGainValue = view.findViewById<TextView>(R.id.output_gain_value)
        val outputGainSlider = view.findViewById<Slider>(R.id.output_gain_slider)
        outputGainSlider.addOnChangeListener { _, value, _ ->
            outputGainValue.text = "%.1f dB".format(value)
            beatriceEngine.setOutputGain(value.toDouble())
        }
        outputGainValue.text = "%.1f dB".format(outputGainSlider.value)

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

        // VQ Neighbors
        val vqValue = view.findViewById<TextView>(R.id.vq_neighbors_value)
        val vqSlider = view.findViewById<Slider>(R.id.vq_neighbors_slider)
        vqSlider.addOnChangeListener { _, value, _ ->
            val intValue = value.toInt()
            vqValue.text = intValue.toString()
            beatriceEngine.setVQNumNeighbors(intValue)
        }
        vqValue.text = vqSlider.value.toInt().toString()
    }
}
