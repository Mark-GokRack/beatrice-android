package com.gokrack.beatriceapp

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.AutoCompleteTextView
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.gokrack.beatriceapp.view.EqCurveView
import com.gokrack.beatriceapp.view.GainReductionBarView
import com.gokrack.beatriceapp.view.PeakMeterView
import androidx.appcompat.widget.SwitchCompat
import com.google.android.material.slider.Slider
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt

class EffectorFragment : Fragment() {

    companion object {
        private const val METER_UPDATE_INTERVAL_MS = 33L
        private const val MIN_HZ = 20.0
        private const val MAX_HZ = 20000.0
        private const val EQ_LOG_STEPS = 1000f
        private const val PRE_EQ_BANDS = 3
        private const val POST_EQ_BANDS = 5

        private val FILTER_TYPES = listOf("Peaking", "Lowpass", "Highpass", "LowShelf", "HighShelf", "Notch", "Allpass")

    }

    private val handler = Handler(Looper.getMainLooper())
    private var isMeterUpdating = false
    private var isRestoring = false

    private val meterRunnable = object : Runnable {
        override fun run() {
            updateMeters()
            if (isMeterUpdating) {
                handler.postDelayed(this, METER_UPDATE_INTERVAL_MS)
            }
        }
    }

    // Sections
    private lateinit var amplifierSection: EffectorSection
    private lateinit var noiseGateSection: EffectorSection
    private lateinit var compressorSection: EffectorSection
    private lateinit var preEqSection: EffectorSection
    private lateinit var postEqSection: EffectorSection
    private lateinit var limiterSection: EffectorSection

    // Sliders
    private lateinit var amplifierGain: SliderBinding

    private lateinit var noiseGateThreshold: SliderBinding
    private lateinit var noiseGateRange: SliderBinding
    private lateinit var noiseGateAttack: SliderBinding
    private lateinit var noiseGateRelease: SliderBinding

    private lateinit var compressorThreshold: SliderBinding
    private lateinit var compressorRatio: SliderBinding
    private lateinit var compressorAttack: SliderBinding
    private lateinit var compressorRelease: SliderBinding
    private lateinit var compressorMakeupGain: SliderBinding

    private lateinit var limiterThreshold: SliderBinding
    private lateinit var limiterAttack: SliderBinding
    private lateinit var limiterRelease: SliderBinding

    // Meters
    private lateinit var noiseGateMeters: DynamicMetersBinding
    private lateinit var compressorMeters: DynamicMetersBinding
    private lateinit var limiterMeters: DynamicMetersBinding

    // EQ
    private lateinit var preEqCurve: EqCurveView
    private lateinit var postEqCurve: EqCurveView
    private val preEqBands = mutableListOf<EqBandBinding>()
    private val postEqBands = mutableListOf<EqBandBinding>()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_effector, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupSections(view)
        setupSliders(view)
        setupMeters(view)
        setupEq(view)

        val viewModel = ViewModelProvider(requireActivity())[EngineStateViewModel::class.java]
        viewModel.settingsResetTrigger.observe(viewLifecycleOwner) {
            isRestoring = true
            applyCurrentSettings()
            isRestoring = false
        }

        isRestoring = true
        applyCurrentSettings()
        isRestoring = false
    }

    override fun onResume() {
        super.onResume()
        startMeterUpdate()
    }

    override fun onPause() {
        super.onPause()
        stopMeterUpdate()
    }

    private fun setupSections(view: View) {
        amplifierSection = EffectorSection(
            view, R.id.amplifier_section_header, R.id.amplifier_content,
            getString(R.string.effector_amplifier), false
        ) { enabled ->
            if (!isRestoring) {
                SettingsManager.saveAmplifierEnabled(enabled)
                beatriceEngine.setAmplifierEnabled(enabled)
            }
        }

        noiseGateSection = EffectorSection(
            view, R.id.noise_gate_section_header, R.id.noise_gate_content,
            getString(R.string.effector_noise_gate), false
        ) { enabled ->
            if (!isRestoring) {
                SettingsManager.saveNoiseGateEnabled(enabled)
                beatriceEngine.setNoiseGateEnabled(enabled)
            }
        }

        compressorSection = EffectorSection(
            view, R.id.compressor_section_header, R.id.compressor_content,
            getString(R.string.effector_compressor), false
        ) { enabled ->
            if (!isRestoring) {
                SettingsManager.saveCompressorEnabled(enabled)
                beatriceEngine.setCompressorEnabled(enabled)
            }
        }

        preEqSection = EffectorSection(
            view, R.id.pre_equalizer_section_header, R.id.pre_equalizer_content,
            getString(R.string.effector_pre_equalizer), false
        ) { enabled ->
            if (!isRestoring) {
                SettingsManager.savePreEqualizerEnabled(enabled)
                beatriceEngine.setPreEqualizerEnabled(enabled)
            }
        }

        postEqSection = EffectorSection(
            view, R.id.post_equalizer_section_header, R.id.post_equalizer_content,
            getString(R.string.effector_post_equalizer), false
        ) { enabled ->
            if (!isRestoring) {
                SettingsManager.savePostEqualizerEnabled(enabled)
                beatriceEngine.setPostEqualizerEnabled(enabled)
            }
        }

        limiterSection = EffectorSection(
            view, R.id.limiter_section_header, R.id.limiter_content,
            getString(R.string.effector_limiter), false
        ) { enabled ->
            if (!isRestoring) {
                SettingsManager.saveLimiterEnabled(enabled)
                beatriceEngine.setLimiterEnabled(enabled)
            }
        }
    }

    private fun setupSliders(view: View) {
        amplifierGain = SliderBinding(
            view, R.id.amplifier_gain_slider,
            getString(R.string.gain), -30f, 30f, 0.1f, "%.1f dB"
        ) { value ->
            SettingsManager.saveAmplifierGain(value)
            beatriceEngine.setAmplifierGain(value.toDouble())
        }

        noiseGateThreshold = SliderBinding(
            view, R.id.noise_gate_threshold_slider,
            getString(R.string.threshold), -60f, 6f, 0.1f, "%.1f dB"
        ) { value ->
            SettingsManager.saveNoiseGateThreshold(value)
            beatriceEngine.setNoiseGateThreshold(value.toDouble())
        }
        noiseGateRange = SliderBinding(
            view, R.id.noise_gate_range_slider,
            getString(R.string.range), -80f, 0f, 0.1f, "%.1f dB"
        ) { value ->
            SettingsManager.saveNoiseGateRange(value)
            beatriceEngine.setNoiseGateRange(value.toDouble())
        }
        noiseGateAttack = SliderBinding(
            view, R.id.noise_gate_attack_slider,
            getString(R.string.attack), 0f, 100f, 0.1f, "%.1f ms"
        ) { value ->
            SettingsManager.saveNoiseGateAttack(value)
            beatriceEngine.setNoiseGateAttack(value.toDouble())
        }
        noiseGateRelease = SliderBinding(
            view, R.id.noise_gate_release_slider,
            getString(R.string.release), 0f, 1000f, 1f, "%.0f ms"
        ) { value ->
            SettingsManager.saveNoiseGateRelease(value)
            beatriceEngine.setNoiseGateRelease(value.toDouble())
        }

        compressorThreshold = SliderBinding(
            view, R.id.compressor_threshold_slider,
            getString(R.string.threshold), -60f, 6f, 0.1f, "%.1f dB"
        ) { value ->
            SettingsManager.saveCompressorThreshold(value)
            beatriceEngine.setCompressorThreshold(value.toDouble())
        }
        compressorRatio = SliderBinding(
            view, R.id.compressor_ratio_slider,
            getString(R.string.ratio), 0f, 30f, 0.1f, "%.1f : 1"
        ) { value ->
            SettingsManager.saveCompressorRatio(value)
            beatriceEngine.setCompressorRatio(value.toDouble())
        }
        compressorAttack = SliderBinding(
            view, R.id.compressor_attack_slider,
            getString(R.string.attack), 0f, 100f, 0.1f, "%.1f ms"
        ) { value ->
            SettingsManager.saveCompressorAttack(value)
            beatriceEngine.setCompressorAttack(value.toDouble())
        }
        compressorRelease = SliderBinding(
            view, R.id.compressor_release_slider,
            getString(R.string.release), 0f, 1000f, 1f, "%.0f ms"
        ) { value ->
            SettingsManager.saveCompressorRelease(value)
            beatriceEngine.setCompressorRelease(value.toDouble())
        }
        compressorMakeupGain = SliderBinding(
            view, R.id.compressor_makeup_gain_slider,
            getString(R.string.makeup_gain), 0f, 30f, 0.1f, "%.1f dB"
        ) { value ->
            SettingsManager.saveCompressorMakeupGain(value)
            beatriceEngine.setCompressorMakeupGain(value.toDouble())
        }

        limiterThreshold = SliderBinding(
            view, R.id.limiter_threshold_slider,
            getString(R.string.threshold), -60f, 6f, 0.1f, "%.1f dB"
        ) { value ->
            SettingsManager.saveLimiterThreshold(value)
            beatriceEngine.setLimiterThreshold(value.toDouble())
        }
        limiterAttack = SliderBinding(
            view, R.id.limiter_attack_slider,
            getString(R.string.attack), 0f, 100f, 0.1f, "%.1f ms"
        ) { value ->
            SettingsManager.saveLimiterAttack(value)
            beatriceEngine.setLimiterAttack(value.toDouble())
        }
        limiterRelease = SliderBinding(
            view, R.id.limiter_release_slider,
            getString(R.string.release), 0f, 1000f, 1f, "%.0f ms"
        ) { value ->
            SettingsManager.saveLimiterRelease(value)
            beatriceEngine.setLimiterRelease(value.toDouble())
        }
    }

    private fun setupMeters(view: View) {
        noiseGateMeters = DynamicMetersBinding(view, R.id.noise_gate_meters)
        compressorMeters = DynamicMetersBinding(view, R.id.compressor_meters)
        limiterMeters = DynamicMetersBinding(view, R.id.limiter_meters)
    }

    private fun setupEq(view: View) {
        preEqCurve = view.findViewById(R.id.pre_equalizer_curve)
        postEqCurve = view.findViewById(R.id.post_equalizer_curve)

        preEqBands.clear()
        for (i in 0 until PRE_EQ_BANDS) {
            val bandView = view.findViewById<View>(getEqBandViewId(true, i))
            preEqBands.add(EqBandBinding(bandView, true, i))
        }

        postEqBands.clear()
        for (i in 0 until POST_EQ_BANDS) {
            val bandView = view.findViewById<View>(getEqBandViewId(false, i))
            postEqBands.add(EqBandBinding(bandView, false, i))
        }
    }

    private fun getEqBandViewId(isPre: Boolean, band: Int): Int {
        return when {
            isPre && band == 0 -> R.id.pre_eq_band_0
            isPre && band == 1 -> R.id.pre_eq_band_1
            isPre && band == 2 -> R.id.pre_eq_band_2
            !isPre && band == 0 -> R.id.post_eq_band_0
            !isPre && band == 1 -> R.id.post_eq_band_1
            !isPre && band == 2 -> R.id.post_eq_band_2
            !isPre && band == 3 -> R.id.post_eq_band_3
            !isPre && band == 4 -> R.id.post_eq_band_4
            else -> View.NO_ID
        }
    }

    private fun applyCurrentSettings() {
        // Amplifier
        val amplifierEnabled = SettingsManager.loadAmplifierEnabled()
        amplifierSection.setChecked(amplifierEnabled)
        val amplifierGainValue = SettingsManager.loadAmplifierGain()
        amplifierGain.setValue(amplifierGainValue)
        beatriceEngine.setAmplifierEnabled(amplifierEnabled)
        beatriceEngine.setAmplifierGain(amplifierGainValue.toDouble())

        // Noise Gate
        val noiseGateEnabled = SettingsManager.loadNoiseGateEnabled()
        noiseGateSection.setChecked(noiseGateEnabled)
        noiseGateThreshold.setValue(SettingsManager.loadNoiseGateThreshold())
        noiseGateRange.setValue(SettingsManager.loadNoiseGateRange())
        noiseGateAttack.setValue(SettingsManager.loadNoiseGateAttack())
        noiseGateRelease.setValue(SettingsManager.loadNoiseGateRelease())
        beatriceEngine.setNoiseGateEnabled(noiseGateEnabled)
        beatriceEngine.setNoiseGateThreshold(noiseGateThreshold.value.toDouble())
        beatriceEngine.setNoiseGateRange(noiseGateRange.value.toDouble())
        beatriceEngine.setNoiseGateAttack(noiseGateAttack.value.toDouble())
        beatriceEngine.setNoiseGateRelease(noiseGateRelease.value.toDouble())

        // Compressor
        val compressorEnabled = SettingsManager.loadCompressorEnabled()
        compressorSection.setChecked(compressorEnabled)
        compressorThreshold.setValue(SettingsManager.loadCompressorThreshold())
        compressorRatio.setValue(SettingsManager.loadCompressorRatio())
        compressorAttack.setValue(SettingsManager.loadCompressorAttack())
        compressorRelease.setValue(SettingsManager.loadCompressorRelease())
        compressorMakeupGain.setValue(SettingsManager.loadCompressorMakeupGain())
        beatriceEngine.setCompressorEnabled(compressorEnabled)
        beatriceEngine.setCompressorThreshold(compressorThreshold.value.toDouble())
        beatriceEngine.setCompressorRatio(compressorRatio.value.toDouble())
        beatriceEngine.setCompressorAttack(compressorAttack.value.toDouble())
        beatriceEngine.setCompressorRelease(compressorRelease.value.toDouble())
        beatriceEngine.setCompressorMakeupGain(compressorMakeupGain.value.toDouble())

        // Limiter
        val limiterEnabled = SettingsManager.loadLimiterEnabled()
        limiterSection.setChecked(limiterEnabled)
        limiterThreshold.setValue(SettingsManager.loadLimiterThreshold())
        limiterAttack.setValue(SettingsManager.loadLimiterAttack())
        limiterRelease.setValue(SettingsManager.loadLimiterRelease())
        beatriceEngine.setLimiterEnabled(limiterEnabled)
        beatriceEngine.setLimiterThreshold(limiterThreshold.value.toDouble())
        beatriceEngine.setLimiterAttack(limiterAttack.value.toDouble())
        beatriceEngine.setLimiterRelease(limiterRelease.value.toDouble())

        // EQ
        val preEqEnabled = SettingsManager.loadPreEqualizerEnabled()
        preEqSection.setChecked(preEqEnabled)
        beatriceEngine.setPreEqualizerEnabled(preEqEnabled)
        preEqBands.forEachIndexed { index, band ->
            val saved = SettingsManager.loadEqualizerBand(true, index)
            band.applyFromSettings(saved)
        }
        updateEqCurve(true)

        val postEqEnabled = SettingsManager.loadPostEqualizerEnabled()
        postEqSection.setChecked(postEqEnabled)
        beatriceEngine.setPostEqualizerEnabled(postEqEnabled)
        postEqBands.forEachIndexed { index, band ->
            val saved = SettingsManager.loadEqualizerBand(false, index)
            band.applyFromSettings(saved)
        }
        updateEqCurve(false)
    }

    private fun updateMeters() {
        noiseGateMeters.setLevels(
            beatriceEngine.getNoiseGateInputPeak().toFloat(),
            beatriceEngine.getNoiseGateOutputPeak().toFloat(),
            beatriceEngine.getNoiseGateGainReduction().toFloat()
        )

        compressorMeters.setLevels(
            beatriceEngine.getCompressorInputPeak().toFloat(),
            beatriceEngine.getCompressorOutputPeak().toFloat(),
            beatriceEngine.getCompressorGainReduction().toFloat()
        )

        limiterMeters.setLevels(
            beatriceEngine.getLimiterInputPeak().toFloat(),
            beatriceEngine.getLimiterOutputPeak().toFloat(),
            beatriceEngine.getLimiterGainReduction().toFloat()
        )
    }

    private fun startMeterUpdate() {
        if (!isMeterUpdating) {
            isMeterUpdating = true
            handler.post(meterRunnable)
        }
    }

    private fun stopMeterUpdate() {
        isMeterUpdating = false
        handler.removeCallbacks(meterRunnable)
    }

    private fun updateEqCurve(isPre: Boolean) {
        val frequencies = beatriceEngine.generateLogFrequencies(256, MIN_HZ, MAX_HZ)
        val response = if (isPre) {
            beatriceEngine.getPreEqualizerFrequencyResponse(frequencies)
        } else {
            beatriceEngine.getPostEqualizerFrequencyResponse(frequencies)
        }
        if (isPre) {
            preEqCurve.setData(frequencies, response)
        } else {
            postEqCurve.setData(frequencies, response)
        }
    }

    private fun applyEqBand(isPre: Boolean, band: Int, type: Int, freq: Float, q: Float, gain: Float) {
        val index = band
        when (type) {
            0 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsPeaking(index, freq.toDouble(), q.toDouble(), gain.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsPeaking(index, freq.toDouble(), q.toDouble(), gain.toDouble())
            }
            1 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsLowpass(index, freq.toDouble(), q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsLowpass(index, freq.toDouble(), q.toDouble())
            }
            2 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsHighpass(index, freq.toDouble(), q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsHighpass(index, freq.toDouble(), q.toDouble())
            }
            3 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsLowShelf(index, freq.toDouble(), q.toDouble(), gain.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsLowShelf(index, freq.toDouble(), q.toDouble(), gain.toDouble())
            }
            4 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsHighShelf(index, freq.toDouble(), q.toDouble(), gain.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsHighShelf(index, freq.toDouble(), q.toDouble(), gain.toDouble())
            }
            5 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsNotch(index, freq.toDouble(), q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsNotch(index, freq.toDouble(), q.toDouble())
            }
            6 -> if (isPre) {
                beatriceEngine.setPreEqualizerBandAsAllpass(index, freq.toDouble(), q.toDouble())
            } else {
                beatriceEngine.setPostEqualizerBandAsAllpass(index, freq.toDouble(), q.toDouble())
            }
        }
        SettingsManager.saveEqualizerBand(isPre, band, type, freq, q, gain)
        updateEqCurve(isPre)
    }

    //region UI helpers

    private inner class EffectorSection(
        view: View,
        headerId: Int,
        private val contentId: Int,
        title: String,
        initialExpanded: Boolean,
        private val onCheckedChanged: (Boolean) -> Unit
    ) {
        private val header = view.findViewById<LinearLayout>(headerId)
        private val content = view.findViewById<View>(contentId)
        private val titleView = header.findViewById<TextView>(R.id.section_title)
        private val arrowView = header.findViewById<TextView>(R.id.section_arrow)
        private val switchView = header.findViewById<SwitchCompat>(R.id.section_switch)

        init {
            titleView.text = title
            switchView.setOnCheckedChangeListener { _, isChecked ->
                if (!isRestoring) {
                    onCheckedChanged(isChecked)
                }
            }
            header.setOnClickListener {
                toggle()
            }
            setExpanded(initialExpanded)
        }

        fun setChecked(checked: Boolean) {
            switchView.isChecked = checked
        }

        fun setExpanded(expanded: Boolean) {
            content.isVisible = expanded
            arrowView.text = if (expanded) "▲" else "▼"
        }

        private fun toggle() {
            setExpanded(!content.isVisible)
        }
    }

    private inner class SliderBinding(
        view: View,
        rootId: Int,
        label: String,
        private val valueFrom: Float,
        private val valueTo: Float,
        private val stepSize: Float,
        private val format: String,
        private val valueTransform: (Float) -> Float = { it },
        private val displayTransform: (Float) -> Float = { it },
        private val onChange: (Float) -> Unit
    ) {
        private val root = view.findViewById<View>(rootId)
        private val labelView = root.findViewById<TextView>(R.id.effector_slider_label)
        private val valueView = root.findViewById<TextView>(R.id.effector_slider_value)
        private val slider = root.findViewById<Slider>(R.id.effector_slider)
        val value: Float get() = slider.value

        init {
            labelView.text = label
            slider.valueFrom = valueFrom
            slider.valueTo = valueTo
            slider.stepSize = stepSize
            root.findViewById<Button>(R.id.effector_slider_decrement).setOnClickListener {
                slider.value = (slider.value - slider.stepSize).coerceAtLeast(valueFrom)
            }
            root.findViewById<Button>(R.id.effector_slider_increment).setOnClickListener {
                slider.value = (slider.value + slider.stepSize).coerceAtMost(valueTo)
            }
            slider.addOnChangeListener { _, newValue, _ ->
                valueView.text = format.format(displayTransform(newValue))
                if (!isRestoring) {
                    onChange(valueTransform(newValue))
                }
            }
        }

        fun setValue(newValue: Float) {
            slider.value = roundToStep(newValue, stepSize).coerceIn(valueFrom, valueTo)
            valueView.text = format.format(displayTransform(slider.value))
        }

        private fun roundToStep(value: Float, step: Float): Float {
            if (step == 0f) return value
            return (value / step).roundToInt() * step
        }
    }

    private inner class DynamicMetersBinding(
        view: View,
        rootId: Int
    ) {
        private val root = view.findViewById<View>(rootId)
        private val inputMeter = root.findViewById<PeakMeterView>(R.id.input_peak_meter)
        private val outputMeter = root.findViewById<PeakMeterView>(R.id.output_peak_meter)
        private val grBar = root.findViewById<GainReductionBarView>(R.id.gain_reduction_bar)
        private val tickMeter = root.findViewById<PeakMeterView>(R.id.tick_meter)

        init {
            inputMeter.setShowTicks(false)
            outputMeter.setShowTicks(false)
            tickMeter.setShowTicks(true)
            tickMeter.setLevelDb(PeakMeterView.MIN_DB)
        }

        fun setLevels(inputDb: Float, outputDb: Float, gainReductionDb: Float) {
            inputMeter.setLevelDb(inputDb)
            outputMeter.setLevelDb(outputDb)
            grBar.setGainReductionDb(gainReductionDb)
        }
    }

    private inner class EqBandBinding(
        private val rootView: View,
        private val isPre: Boolean,
        private val bandIndex: Int
    ) {
        private val titleView = rootView.findViewById<TextView>(R.id.eq_band_title)
        private val arrowView = rootView.findViewById<TextView>(R.id.eq_band_arrow)
        private val controlsView = rootView.findViewById<View>(R.id.eq_band_controls)
        private val headerView = rootView.findViewById<View>(R.id.eq_band_header)
        private val typeView = rootView.findViewById<AutoCompleteTextView>(R.id.eq_band_type)
        private val frequencySlider = SliderBinding(
            rootView, R.id.eq_band_frequency,
            getString(R.string.frequency), 0f, EQ_LOG_STEPS, 1f, "%.0f Hz",
            valueTransform = { progressToHz(it) },
            displayTransform = { progressToHz(it) }
        ) { _ -> applyBand() }
        private val qSlider = SliderBinding(
            rootView, R.id.eq_band_q,
            getString(R.string.q_value), 0.1f, 10f, 0.01f, "%.2f"
        ) { _ -> applyBand() }
        private val gainSlider = SliderBinding(
            rootView, R.id.eq_band_gain,
            getString(R.string.gain), -30f, 30f, 0.1f, "%.1f dB"
        ) { _ -> applyBand() }

        private var currentType = 0

        init {
            titleView.text = "Band ${bandIndex + 1}"
            setExpanded(false)
            headerView.setOnClickListener {
                setExpanded(!controlsView.isVisible)
            }
            val adapter = ArrayAdapter(requireContext(), android.R.layout.simple_dropdown_item_1line, FILTER_TYPES)
            typeView.setAdapter(adapter)
            typeView.onItemClickListener = AdapterView.OnItemClickListener { _, _, position, _ ->
                currentType = position
                updateGainVisibility()
                applyBand()
            }
            currentType = 0
            typeView.setText(FILTER_TYPES[currentType], false)
            updateGainVisibility()
        }

        fun applyFromSettings(settings: SettingsManager.EqualizerBandSettings) {
            currentType = settings.type.coerceIn(0, FILTER_TYPES.size - 1)
            typeView.setText(FILTER_TYPES[currentType], false)
            frequencySlider.setValue(hzToProgress(settings.frequency))
            qSlider.setValue(settings.q)
            gainSlider.setValue(settings.gain)
            updateGainVisibility()
            applyEqBand(isPre, bandIndex, currentType, settings.frequency, settings.q, settings.gain)
        }

        private fun setExpanded(expanded: Boolean) {
            controlsView.isVisible = expanded
            arrowView.text = if (expanded) "▲" else "▼"
        }

        private fun applyBand() {
            val freq = progressToHz(frequencySlider.value)
            val q = qSlider.value
            val gain = gainSlider.value
            applyEqBand(isPre, bandIndex, currentType, freq, q, gain)
        }

        private fun updateGainVisibility() {
            val needsGain = currentType == 0 || currentType == 3 || currentType == 4
            val gainRoot = rootView.findViewById<View>(R.id.eq_band_gain)
            gainRoot.isVisible = needsGain
        }

        private fun progressToHz(progress: Float): Float {
            val ratio = progress / EQ_LOG_STEPS
            return exp(ln(MIN_HZ) + ratio * (ln(MAX_HZ) - ln(MIN_HZ))).toFloat()
        }

        private fun hzToProgress(hz: Float): Float {
            val ratio = (ln(hz.toDouble()) - ln(MIN_HZ)) / (ln(MAX_HZ) - ln(MIN_HZ))
            return (ratio * EQ_LOG_STEPS).toFloat().coerceIn(0f, EQ_LOG_STEPS)
        }
    }

    //endregion
}
