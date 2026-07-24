package com.gokrack.beatriceapp.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.util.AttributeSet
import android.view.View
import androidx.core.content.ContextCompat
import com.gokrack.beatriceapp.R
import kotlin.math.log10
import kotlin.math.max
import kotlin.math.min

/**
 * Draws a parametric equalizer frequency response curve.
 * X axis: logarithmic frequency [20 Hz, 20000 Hz]
 * Y axis: magnitude in dB [-96 dB, +24 dB]
 */
class EqCurveView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    companion object {
        const val MIN_HZ = 20.0
        const val MAX_HZ = 20000.0
        const val MIN_DB = -30.0
        const val MAX_DB = 30.0
    }

    private val gridPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_divider)
        strokeWidth = 1f
        style = Paint.Style.STROKE
    }
    private val gridLabelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_text_secondary)
        textSize = context.resources.getDimension(R.dimen.meter_tick_text_size)
        textAlign = Paint.Align.RIGHT
    }
    private val curvePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_accent_teal)
        strokeWidth = 3f
        style = Paint.Style.STROKE
    }
    private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_accent_teal)
        alpha = 40
        style = Paint.Style.FILL
    }

    private var frequencies: DoubleArray = doubleArrayOf()
    private var magnitudes: DoubleArray = doubleArrayOf()
    private val path = Path()
    private val fillPath = Path()

    fun setData(frequencies: DoubleArray, magnitudes: DoubleArray) {
        if (frequencies.size == magnitudes.size) {
            this.frequencies = frequencies
            this.magnitudes = magnitudes
            invalidate()
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val paddingLeft = 44f
        val paddingRight = 12f
        val paddingTop = 16f
        val paddingBottom = 28f
        val chartLeft = paddingLeft
        val chartTop = paddingTop
        val chartRight = width - paddingRight
        val chartBottom = height - paddingBottom
        val chartWidth = chartRight - chartLeft
        val chartHeight = chartBottom - chartTop

        // Draw grid lines and labels
        val freqs = listOf(20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000)
        freqs.forEach { freq ->
            val x = chartLeft + chartWidth * ((log10(freq.toDouble()) - log10(MIN_HZ)) / (log10(MAX_HZ) - log10(MIN_HZ))).toFloat()
            canvas.drawLine(x, chartTop, x, chartBottom, gridPaint)
            val label = if (freq >= 1000) "${freq / 1000}k" else "$freq"
            canvas.drawText(label, x, height - 8f, gridLabelPaint)
        }

        // Horizontal grid lines and dB labels every 6 dB
        var db = MIN_DB.toFloat()
        while (db <= MAX_DB + 0.01f) {
            val y = chartBottom - chartHeight * ((db - MIN_DB) / (MAX_DB - MIN_DB)).toFloat()
            canvas.drawLine(chartLeft, y, chartRight, y, gridPaint)
            val labelX = chartLeft - 6f
            val labelY = y + gridLabelPaint.textSize / 3f
            canvas.drawText("%.0f".format(db), labelX, labelY, gridLabelPaint)
            db += 6f
        }

        if (frequencies.isEmpty() || magnitudes.isEmpty()) {
            return
        }

        // Build path
        path.reset()
        fillPath.reset()
        var first = true
        val zeroY = chartBottom - chartHeight * ((0.0 - MIN_DB) / (MAX_DB - MIN_DB)).toFloat()

        for (i in frequencies.indices) {
            val freq = frequencies[i]
            val db = magnitudes[i]
            if (freq <= 0) continue
            val x = chartLeft + chartWidth * ((log10(freq) - log10(MIN_HZ)) / (log10(MAX_HZ) - log10(MIN_HZ))).toFloat()
            val clampedDb = max(MIN_DB, min(MAX_DB, db))
            val y = chartBottom - chartHeight * ((clampedDb - MIN_DB) / (MAX_DB - MIN_DB)).toFloat()
            if (first) {
                path.moveTo(x, y)
                fillPath.moveTo(x, zeroY)
                fillPath.lineTo(x, y)
                first = false
            } else {
                path.lineTo(x, y)
                fillPath.lineTo(x, y)
            }
        }
        if (!first) {
            fillPath.lineTo(chartRight, zeroY)
            fillPath.close()
            canvas.drawPath(fillPath, fillPaint)
            canvas.drawPath(path, curvePaint)
        }
    }
}
