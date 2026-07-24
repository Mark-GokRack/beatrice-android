package com.gokrack.beatriceapp.view

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View
import androidx.core.content.ContextCompat
import com.gokrack.beatriceapp.R
import kotlin.math.max
import kotlin.math.min

/**
 * Horizontal dB meter visualizing levels in the range [-60, 6] dB.
 */
class PeakMeterView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    companion object {
        const val MIN_DB = -60f
        const val MAX_DB = 6f
        private const val TICK_STEP_DB = 6f
    }

    private val greenPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.meter_green)
        style = Paint.Style.FILL
    }
    private val yellowPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.meter_yellow)
        style = Paint.Style.FILL
    }
    private val redPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.meter_red)
        style = Paint.Style.FILL
    }
    private val bgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_bg_card)
        style = Paint.Style.FILL
    }
    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_divider)
        style = Paint.Style.STROKE
        strokeWidth = 2f
    }
    private val tickPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_text_secondary)
        strokeWidth = 1f
    }
    private val tickLabelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.vst_text_secondary)
        textSize = context.resources.getDimension(R.dimen.meter_tick_text_size)
        textAlign = Paint.Align.CENTER
    }

    private var currentDb: Float = MIN_DB
    private var showTicks: Boolean = true
    private val barRect = RectF()
    private val bgRect = RectF()

    fun setLevelDb(db: Float) {
        currentDb = max(MIN_DB, min(MAX_DB, db))
        invalidate()
    }

    fun setShowTicks(show: Boolean) {
        showTicks = show
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val padding = 4f
        val top = padding
        val bottom = if (showTicks) {
            height - padding - tickLabelPaint.textSize - 8f
        } else {
            height - padding
        }
        val left = padding
        val right = width - padding

        val fraction = (currentDb - MIN_DB) / (MAX_DB - MIN_DB)
        val fillRight = left + (right - left) * fraction.coerceIn(0f, 1f)


        // Ticks every 6 dB (shared with another meter, so can be hidden)
        if (showTicks) {
            var tickDb = MIN_DB
            while (tickDb <= MAX_DB + 0.01f) {
                val tickFraction = (tickDb - MIN_DB) / (MAX_DB - MIN_DB)
                val x = left + (right - left) * tickFraction
                canvas.drawLine(x, bottom + 4f, x, bottom + 12f, tickPaint)
                canvas.drawText("%.0f".format(tickDb), x, height - padding, tickLabelPaint)
                tickDb += TICK_STEP_DB
            }
        }else{
            bgRect.set(left, top, right, bottom)
            canvas.drawRoundRect(bgRect, 4f, 4f, bgPaint)
            canvas.drawRoundRect(bgRect, 4f, 4f, borderPaint)

            barRect.set(left, top, fillRight, bottom)
            val paint = when {
                currentDb >= -6f -> redPaint
                currentDb >= -18f -> yellowPaint
                else -> greenPaint
            }
            canvas.drawRoundRect(barRect, 4f, 4f, paint)
        }
    }
}
