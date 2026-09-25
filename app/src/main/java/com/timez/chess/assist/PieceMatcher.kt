package com.timez.chess.assist

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import com.timez.chess.gamelogic.Piece
import kotlin.collections.iterator
import kotlin.math.sqrt

/**
 * 模板匹配棋子识别器。
 * YOLO 只负责给出棋盘线框位置，本类负责在框内 9×10 个交叉点上，
 * 用归一化互相关（NCC）比对 14 个棋子模板，输出 canonical 棋盘。
 */
class PieceMatcher(private val context: Context) {

    companion object {
        private const val TAG = "PieceMatcher"
        private const val TSIZE = 48
        private const val MATCH_THRESHOLD = 0.50f

        private val CLASSES = arrayOf(
            "r_che", "r_ma", "r_xiang", "r_shi", "r_shuai", "r_pao", "r_bing",
            "b_che", "b_ma", "b_xiang", "b_shi", "b_jiang", "b_pao", "b_zu"
        )
        private val SKINS = arrayOf("wood", "green", "white")

        private val CLASS_TO_PIECE: Map<String, Int> = mapOf(
            "r_che" to Piece.WJU, "r_ma" to Piece.WMA, "r_xiang" to Piece.WXIANG,
            "r_shi" to Piece.WSHI, "r_shuai" to Piece.WSHUAI, "r_pao" to Piece.WPAO,
            "r_bing" to Piece.WBING,
            "b_che" to Piece.BJU, "b_ma" to Piece.BMA, "b_xiang" to Piece.BXIANG,
            "b_shi" to Piece.BSHI, "b_jiang" to Piece.BJIANG, "b_pao" to Piece.BPAO,
            "b_zu" to Piece.BZU,
        )
    }

    private val templates: Map<String, Map<String, FloatArray>>

    init {
        val loaded = mutableMapOf<String, Map<String, FloatArray>>()
        for (skin in SKINS) {
            val skinMap = mutableMapOf<String, FloatArray>()
            for (cls in CLASSES) {
                try {
                    context.assets.open("templates/$skin/$cls.png").use { ins ->
                        val bmp = BitmapFactory.decodeStream(ins)
                        if (bmp != null) {
                            skinMap[cls] = extractGrayFromBitmap(bmp)
                            bmp.recycle()
                        }
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "load failed: $skin/$cls: ${e.message}")
                }
            }
            if (skinMap.size >= 12) {
                loaded[skin] = skinMap
                Log.i(TAG, "loaded skin $skin: ${skinMap.size} classes")
            }
        }
        templates = loaded
        Log.i(TAG, "available skins: ${templates.keys}")
    }

    data class MatchResult(
        val canonical: Array<IntArray>,
        val skin: String,
        val score: Float,
        val pieceCount: Int,
    )

    /**
     * @param nx0,ny0,nx1,ny1 棋盘线框归一化坐标（左上、右下）
     * @param preferredSkin 指定皮肤（"wood"/"green"/"white"）；null 或 "auto" 时跑全部皮肤取最优
     */
    fun match(
        frame: Frame,
        nx0: Float, ny0: Float, nx1: Float, ny1: Float,
        preferredSkin: String? = null,
    ): MatchResult? {
        if (templates.isEmpty()) return null

        val x0 = (nx0 * frame.width).toInt()
        val y0 = (ny0 * frame.height).toInt()
        val x1 = (nx1 * frame.width).toInt()
        val y1 = (ny1 * frame.height).toInt()
        if (x1 - x0 < 100 || y1 - y0 < 100) return null

        val colGap = (x1 - x0) / 8f
        val rowGap = (y1 - y0) / 9f
        val half = (minOf(colGap, rowGap) * 0.40f).toInt()

        val skinList: List<String> = if (!preferredSkin.isNullOrEmpty() && preferredSkin != "auto"
            && templates.containsKey(preferredSkin)) {
            listOf(preferredSkin)
        } else {
            templates.keys.toList()
        }

        var best: MatchResult? = null
        for (skin in skinList) {
            val tplMap = templates[skin] ?: continue
            val board = Array(10) { IntArray(9) }
            var scoreSum = 0f
            var count = 0
            for (row in 0 until 10) {
                for (col in 0 until 9) {
                    val cx = (x0 + col * colGap).toInt()
                    val cy = (y0 + row * rowGap).toInt()
                    val sx = (cx - half).coerceAtLeast(0)
                    val sy = (cy - half).coerceAtLeast(0)
                    val ex = (cx + half).coerceAtMost(frame.width)
                    val ey = (cy + half).coerceAtMost(frame.height)
                    if (ex - sx < 8 || ey - sy < 8) continue

                    val gray = extractGrayFromFrame(frame, sx, sy, ex, ey)
                    var bestCls: String? = null
                    var bestScore = -1f
                    for ((cls, tpl) in tplMap) {
                        val s = ncc(gray, tpl)
                        if (s > bestScore) { bestScore = s; bestCls = cls }
                    }
                    if (bestCls != null && bestScore > MATCH_THRESHOLD) {
                        board[row][col] = CLASS_TO_PIECE[bestCls] ?: Piece.EMPTY
                        scoreSum += bestScore
                        count++
                    }
                }
            }
            val avg = if (count > 0) scoreSum / count else 0f
            val r = MatchResult(board, skin, avg, count)
            if (best == null || r.pieceCount > best!!.pieceCount) best = r
        }
        return best
    }

    /** 从帧像素中裁取 x0..x1 / y0..y1 区域，缩放为 TSIZE×TSIZE 灰度归一化数组 */
    private fun extractGrayFromFrame(frame: Frame, x0: Int, y0: Int, x1: Int, y1: Int): FloatArray {
        val w = x1 - x0
        val h = y1 - y0
        val out = FloatArray(TSIZE * TSIZE)
        val stepX = w.toFloat() / TSIZE
        val stepY = h.toFloat() / TSIZE
        for (ty in 0 until TSIZE) {
            val sy = (y0 + ty * stepY).toInt().coerceIn(0, frame.height - 1)
            for (tx in 0 until TSIZE) {
                val sx = (x0 + tx * stepX).toInt().coerceIn(0, frame.width - 1)
                val p = frame.argb[sy * frame.width + sx]
                val r = (p shr 16) and 0xFF
                val g = (p shr 8) and 0xFF
                val b = p and 0xFF
                out[ty * TSIZE + tx] = (0.299f * r + 0.587f * g + 0.114f * b) / 255f
            }
        }
        normalize(out)
        return out
    }

    private fun extractGrayFromBitmap(bmp: Bitmap): FloatArray {
        val scaled = if (bmp.width != TSIZE || bmp.height != TSIZE) {
            Bitmap.createScaledBitmap(bmp, TSIZE, TSIZE, true)
        } else bmp
        val px = IntArray(TSIZE * TSIZE)
        scaled.getPixels(px, 0, TSIZE, 0, 0, TSIZE, TSIZE)
        val out = FloatArray(TSIZE * TSIZE)
        for (i in px.indices) {
            val p = px[i]
            val r = (p shr 16) and 0xFF
            val g = (p shr 8) and 0xFF
            val b = p and 0xFF
            out[i] = (0.299f * r + 0.587f * g + 0.114f * b) / 255f
        }
        if (scaled !== bmp) scaled.recycle()
        normalize(out)
        return out
    }

    private fun normalize(arr: FloatArray) {
        var sum = 0f
        for (v in arr) sum += v
        val mean = sum / arr.size
        var sq = 0f
        for (v in arr) sq += (v - mean) * (v - mean)
        val std = sqrt(sq / arr.size).coerceAtLeast(1e-6f)
        for (i in arr.indices) arr[i] = (arr[i] - mean) / std
    }

    private fun ncc(a: FloatArray, b: FloatArray): Float {
        var dot = 0f
        for (i in a.indices) dot += a[i] * b[i]
        return dot / a.size
    }
}