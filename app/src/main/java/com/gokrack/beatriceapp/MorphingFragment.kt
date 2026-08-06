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
import java.util.Locale

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

        viewModel.morphingVoiceNames.observe(viewLifecycleOwner) { names ->
            viewModel.ensureMorphingWeightsInitialized()
            adapter.setData(names)
        }

        viewModel.settingsResetTrigger.observe(viewLifecycleOwner) {
            val names = viewModel.morphingVoiceNames.value ?: emptyList()
            viewModel.ensureMorphingWeightsInitialized()
            adapter.setData(names)
        }
    }

    companion object {
        private const val PAYLOAD_GRAYOUT = "grayout"
    }

    inner class MorphingAdapter : RecyclerView.Adapter<MorphingAdapter.ViewHolder>() {

        private val voiceNameList = mutableListOf<String>()
        private var voiceCount = 0

        private fun currentWeights(): FloatArray {
            return viewModel.ensureMorphingWeightsInitialized()
        }

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
            val weights = currentWeights()
            val weight = weights[position]
            holder.nameLabel.text = voiceNameList.getOrElse(position) { "Voice $position" }
            holder.valueText.text = String.format(Locale.US, "%.2f", weight)

            holder.slider.clearOnChangeListeners()
            holder.slider.value = weight

            bindGrayout(holder, position)

            holder.slider.addOnChangeListener { _, value, fromUser ->
                if (!fromUser) return@addOnChangeListener
                val adapterPosition = holder.bindingAdapterPosition
                if (adapterPosition == RecyclerView.NO_POSITION) return@addOnChangeListener
                applyWeight(adapterPosition, value, holder)
            }

            holder.decrementBtn.setOnClickListener {
                val adapterPosition = holder.bindingAdapterPosition
                if (adapterPosition == RecyclerView.NO_POSITION) return@setOnClickListener
                val latestWeights = currentWeights()
                val newValue = (Math.round(latestWeights[adapterPosition] * 100) - 1).coerceAtLeast(0) / 100f
                holder.slider.value = newValue
                applyWeight(adapterPosition, newValue, holder)
            }

            holder.incrementBtn.setOnClickListener {
                val adapterPosition = holder.bindingAdapterPosition
                if (adapterPosition == RecyclerView.NO_POSITION) return@setOnClickListener
                val latestWeights = currentWeights()
                val newValue = (Math.round(latestWeights[adapterPosition] * 100) + 1).coerceAtMost(100) / 100f
                holder.slider.value = newValue
                applyWeight(adapterPosition, newValue, holder)
            }
        }

        private fun applyWeight(pos: Int, newVal: Float, holder: ViewHolder) {
            val update = viewModel.applyMorphingWeight(pos, newVal)
            if (!update.changed) return

            holder.valueText.text = String.format(Locale.US, "%.2f", update.roundedValue)

            if (update.zeroStateChanged) {
                holder.slider.post {
                    for (i in 0 until voiceCount) {
                        notifyItemChanged(i, PAYLOAD_GRAYOUT)
                    }
                }
            }
        }

        private fun bindGrayout(holder: ViewHolder, position: Int) {
            val weights = currentWeights()
            val weight = weights[position]
            val enabled = isSliderEnabled(position, weight)
            val alpha = if (!enabled) 0.4f else 1.0f

            holder.nameLabel.alpha = alpha
            holder.valueText.alpha = alpha
            holder.slider.alpha = alpha
            holder.slider.isEnabled = enabled

            val canDecrement = weight > 0.0001f
            holder.decrementBtn.isEnabled = canDecrement
            holder.decrementBtn.alpha = if (canDecrement) 1.0f else alpha

            holder.incrementBtn.isEnabled = enabled && weight < 0.9999f
            holder.incrementBtn.alpha = alpha
        }

        private fun isSliderEnabled(position: Int, currentWeight: Float): Boolean {
            if (beatriceEngine.getModelVersion() < 2) return true
            if (currentWeight > 0.0001f) return true
            return countNonZero() < 8
        }

        private fun countNonZero(): Int {
            val weights = currentWeights()
            return (0 until voiceCount).count { weights[it] > 0.001f }
        }

        fun setData(names: List<String>) {
            voiceNameList.clear()
            voiceNameList.addAll(names)
            voiceCount = names.size
            notifyDataSetChanged()
        }
    }
}
