package com.gokrack.beatriceapp

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.google.android.material.slider.Slider

class GainFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_gain, container, false)

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
