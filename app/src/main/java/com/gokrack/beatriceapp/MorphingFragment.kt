package com.gokrack.beatriceapp

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.slider.Slider

class MorphingFragment : Fragment() {

    private lateinit var viewModel: EngineStateViewModel
    private lateinit var recyclerView: RecyclerView
    private lateinit var adapter: MorphingAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_morphing, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        viewModel = ViewModelProvider(requireActivity())[EngineStateViewModel::class.java]

        recyclerView = view.findViewById(R.id.morphing_recycler_view)
        recyclerView.layoutManager = LinearLayoutManager(requireContext())
        adapter = MorphingAdapter()
        recyclerView.adapter = adapter

        // morphingVoiceNames fires immediately with the current value (including after model load),
        // and again each time a new model is loaded.
        viewModel.morphingVoiceNames.observe(viewLifecycleOwner) { names ->
            val weights = viewModel.morphingWeights.value ?: FloatArray(256)
            adapter.setData(names, weights)
        }
    }

    companion object {
        private const val PAYLOAD_GRAYOUT = "grayout"
    }

    inner class MorphingAdapter : RecyclerView.Adapter<MorphingAdapter.ViewHolder>() {

        private val weights = FloatArray(256) { 0f }
        private val voiceNameList = mutableListOf<String>()
        private var voiceCount = 0

        inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
            val nameLabel: TextView = itemView.findViewById(R.id.voice_name_label)
            val valueText: TextView = itemView.findViewById(R.id.morphing_weight_value)
            val decrementBtn: Button = itemView.findViewById(R.id.morphing_decrement)
            val slider: Slider = itemView.findViewById(R.id.morphing_slider)
            val incrementBtn: Button = itemView.findViewById(R.id.morphing_increment)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val itemView = LayoutInflater.from(parent.context)
                .inflate(R.layout.item_morphing_slider, parent, false)
            return ViewHolder(itemView)
        }

        override fun getItemCount(): Int = voiceCount

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            bindFull(holder, position)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int, payloads: List<Any>) {
            if (payloads.any { it == PAYLOAD_GRAYOUT }) {
                bindGrayout(holder, position)
            } else {
                bindFull(holder, position)
            }
        }

        private fun bindFull(holder: ViewHolder, position: Int) {
            val w = weights[position]
            holder.nameLabel.text = voiceNameList.getOrElse(position) { "Voice $position" }
            holder.valueText.text = "%.1f".format(w)

            // Clear listener before setting value to prevent recursive callbacks during bind
            holder.slider.clearOnChangeListeners()
            holder.slider.value = w

            bindGrayout(holder, position)

            holder.slider.addOnChangeListener { _, value, fromUser ->
                if (!fromUser) return@addOnChangeListener
                val pos = holder.bindingAdapterPosition
                if (pos == RecyclerView.NO_ID.toInt()) return@addOnChangeListener

                val rounded = Math.round(value * 10) / 10f
                val oldWeight = weights[pos]
                if (rounded == oldWeight) return@addOnChangeListener

                val wasZero = oldWeight < 0.001f
                val isNowZero = rounded < 0.001f

                weights[pos] = rounded
                // Mutate ViewModel array in-place (no postValue — avoids observer loop)
                viewModel.morphingWeights.value?.set(pos, rounded)
                beatriceEngine.setSpeakerMorphingWeight(pos, rounded.toDouble())

                holder.valueText.text = "%.1f".format(rounded)

                if (wasZero != isNowZero) {
                    // Non-zero count changed: update grayout for all items after touch completes
                    viewModel.morphingDescriptionTrigger.postValue(Unit)
                    holder.slider.post {
                        for (i in 0 until voiceCount) {
                            notifyItemChanged(i, PAYLOAD_GRAYOUT)
                        }
                    }
                }
            }

            holder.decrementBtn.setOnClickListener {
                val pos = holder.bindingAdapterPosition
                if (pos == RecyclerView.NO_ID.toInt()) return@setOnClickListener
                val rounded = Math.round(weights[pos] * 10)
                val newVal = (rounded - 1).coerceAtLeast(0) / 10f
                holder.slider.value = newVal
            }

            holder.incrementBtn.setOnClickListener {
                val pos = holder.bindingAdapterPosition
                if (pos == RecyclerView.NO_ID.toInt()) return@setOnClickListener
                val rounded = Math.round(weights[pos] * 10)
                val newVal = (rounded + 1).coerceAtMost(10) / 10f
                holder.slider.value = newVal
            }
        }

        private fun bindGrayout(holder: ViewHolder, position: Int) {
            val w = weights[position]
            val enabled = isSliderEnabled(position, w)
            val alpha = if (!enabled) 0.4f else 1.0f

            holder.nameLabel.alpha = alpha
            holder.valueText.alpha = alpha
            holder.slider.alpha = alpha
            holder.slider.isEnabled = enabled

            // Decrement is enabled only when there is a value to decrease (regardless of limit)
            val canDecrement = w > 0.001f
            holder.decrementBtn.isEnabled = canDecrement
            holder.decrementBtn.alpha = if (canDecrement) 1.0f else alpha

            holder.incrementBtn.isEnabled = enabled && w < 0.999f
            holder.incrementBtn.alpha = alpha
        }

        /**
         * A slider at [position] is interactive when:
         * - Model version < 2 (no limit), OR
         * - Its current weight is non-zero (can always adjust or zero it out), OR
         * - The total non-zero count is still below 8 (free slot available)
         */
        private fun isSliderEnabled(position: Int, currentWeight: Float): Boolean {
            if (beatriceEngine.getModelVersion() < 2) return true
            if (currentWeight > 0.001f) return true
            return countNonZero() < 8
        }

        private fun countNonZero(): Int = (0 until voiceCount).count { weights[it] > 0.001f }

        /** Replace all data (called on model load or fragment recreation). */
        fun setData(names: List<String>, newWeights: FloatArray) {
            voiceNameList.clear()
            voiceNameList.addAll(names)
            voiceCount = names.size
            for (i in 0 until 256) {
                weights[i] = newWeights[i]
            }
            notifyDataSetChanged()
        }
    }
}
