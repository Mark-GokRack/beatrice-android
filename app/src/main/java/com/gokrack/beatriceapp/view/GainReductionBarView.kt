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
 * Thin horizontal bar visualizing gain reduction in the range [-60, 0] dB.
 * The bar extends from the 0 dB position (right edge) toward the gain
 * reduction value (left), so more reduction produces a longer cyan bar.
 */
class GainReductionBarView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    companion object {
        const val MIN_DB = -60f
        const val MAX_DB = 6f
    }

    private val barPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = ContextCompat.getColor(context, R.color.gain_reduction_cyan)
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

    private var currentDb: Float = 0f
    private val barRect = RectF()
    private val bgRect = RectF()

    fun setGainReductionDb(db: Float) {
        currentDb = max(MIN_DB, min(MAX_DB, db))
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val padding = 2f
        val top = padding
        val bottom = height - padding
        val left = padding
        val right = width - padding

        bgRect.set(left, top, right, bottom)
        canvas.drawRoundRect(bgRect, 2f, 2f, bgPaint)
        canvas.drawRoundRect(bgRect, 2f, 2f, borderPaint)

        // +6 dB is at the right edge; 0 dB is at a fixed position.
        // The bar extends from the 0 dB position toward the gain reduction value (left).
        val zeroDbFraction = (0f - MIN_DB) / (MAX_DB - MIN_DB)
        val zeroDbRight = left + (right - left) * zeroDbFraction
        val fraction = (currentDb - MIN_DB) / (MAX_DB - MIN_DB)
        val barLeft = min(left + (right - left) * fraction.coerceIn(0f, 1f), zeroDbRight)
        barRect.set(barLeft, top, zeroDbRight, bottom)
        canvas.drawRoundRect(barRect, 2f, 2f, barPaint)
    }
}
