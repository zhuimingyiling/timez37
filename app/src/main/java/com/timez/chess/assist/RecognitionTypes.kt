package com.timez.chess.assist

import kotlin.math.abs
import com.timez.chess.gamelogic.Piece

/**
 * 识别管线的共享类型（纯 JVM，无 Android 依赖）：
 * 帧输入、朝向、单帧识别结果、稳定帧跟踪器。
 */
class Frame(val width: Int, val height: Int, val argb: IntArray) {
    init {
        require(argb.size == width * height) { "frame pixel count mismatch" }
    }
}

enum class Orientation {
    /** 红方在下（标准朝向，屏幕下方阵营=红方） */
    STANDARD,
    /** 红方在上（换边/翻转朝向，屏幕下方阵营=黑方） */
    FLIPPED;
}

/**
 * 一帧识别结果。canonical 为项目内部约定（红恒定在 y=9），orientation 表示屏幕朝向。
 */
class RecognitionResult(
    val canonical: Array<IntArray>,
    val screenRaw: Array<IntArray>,
    val orientation: Orientation,
    val issues: List<String>,
    val unknownCells: Int,
    val recognizedPieces: Int,
    val avgScore: Double,
) {
    fun isValid(): Boolean = issues.isEmpty() && unknownCells == 0
}

/**
 * 稳定帧跟踪器：连续 confirmCount 帧出现同一局面才确认，并推断轮次/检测重开。
 */
class BoardTracker(private val confirmCount: Int = 2) {

    enum class Event {
        NEW_BOARD,
        SAME_BOARD,
        NEW_GAME,
        UNSTABLE,
    }

    var confirmed: RecognitionResult? = null
        private set

    @Volatile var redGo = true

    @Volatile var unstableStreak = 0
        private set

    private var candidate: RecognitionResult? = null
    private var candidateHits = 0
    /** 当前候选对应的“走子起点格”（x, y）；同一步走子才能连续累计 */
    private var candidateStartCell: Pair<Int, Int>? = null

    fun reset(redGoFirst: Boolean = true) {
        confirmed = null
        candidate = null
        candidateHits = 0
        candidateStartCell = null
        unstableStreak = 0
        redGo = redGoFirst
    }

    fun restore(result: RecognitionResult, redGoSide: Boolean) {
        confirmed = result
        redGo = redGoSide
        candidate = null
        candidateHits = 0
        candidateStartCell = null
        unstableStreak = 0
    }

    fun onFrame(res: RecognitionResult): Event {
        val prev = confirmed

        // 不合法帧
        if (!res.isValid()) {
            unstableStreak++
            return Event.UNSTABLE
        }

        // 首次确认
        if (prev == null) {
            if (candidate != null && AssistBoard.equal(res.canonical, candidate!!.canonical)) {
                candidateHits++
            } else {
                candidate = res
                candidateHits = 1
            }
            if (candidateHits >= confirmCount) {
                confirmed = candidate!!
                val first = candidate!!
                candidate = null
                candidateHits = 0
                unstableStreak = 0
                return if (AssistBoard.matchStartCount(first.canonical) >= 32)
                    Event.NEW_GAME else Event.NEW_BOARD
            }
            unstableStreak++
            return Event.UNSTABLE
        }

        // 重开检测
        if (AssistBoard.matchStartCount(res.canonical) >= 32 &&
            !AssistBoard.equal(res.canonical, prev.canonical)) {
            confirmed = res
            redGo = true
            candidate = null
            candidateHits = 0
            candidateStartCell = null
            unstableStreak = 0
            return Event.NEW_GAME
        }

        // 子数跳变门限
        val tol = maxOf(2, prev.recognizedPieces / 10)
        if (abs(res.recognizedPieces - prev.recognizedPieces) > tol) {
            candidate = null
            candidateHits = 0
            candidateStartCell = null
            unstableStreak++
            return Event.UNSTABLE
        }

        // 与已确认局面完全相同
        if (AssistBoard.equal(res.canonical, prev.canonical)) {
            candidate = null
            candidateHits = 0
            candidateStartCell = null
            unstableStreak = 0
            return Event.SAME_BOARD
        }

        // 走子特征比对
        val diff = AssistBoard.moveDiff(res.canonical, prev.canonical)
        val occToEmpty = diff[0]
        val emptyToOcc = diff[1]
        val typeChange = diff[2]

        val isRealMove = (occToEmpty == 1) && (emptyToOcc + typeChange <= 2)

        if (!isRealMove) {
            // 抖动：小范围静默忽略，大范围报不稳定
            if (typeChange <= 4) {
                candidate = null
                candidateHits = 0
                candidateStartCell = null
                unstableStreak = 0
                return Event.SAME_BOARD
            }
            candidate = null
            candidateHits = 0
            candidateStartCell = null
            unstableStreak++
            return Event.UNSTABLE
        }

        // 找出起点格（从有子变空的唯一格子）
        var startCell: Pair<Int, Int>? = null
        for (y in 0 until AssistBoard.H) {
            for (x in 0 until AssistBoard.W) {
                val p0 = prev.canonical[y][x]
                val p1 = res.canonical[y][x]
                if (p0 != Piece.EMPTY && p1 == Piece.EMPTY) {
                    startCell = x to y
                    break
                }
            }
            if (startCell != null) break
        }
        if (startCell == null) {
            unstableStreak++
            return Event.UNSTABLE
        }

        // 候选确认：要求连续 confirmCount 帧都是“同一步走子”（起点格相同）
        if (candidateStartCell == startCell && candidate != null) {
            // 检查这次 res 与 candidate 是否仍是同一步（起点相同、终点一致）
            val stepDiff = AssistBoard.moveDiff(res.canonical, candidate!!.canonical)
            val stillSameStep = stepDiff[0] == 0 && stepDiff[1] == 0 && stepDiff[2] <= 2
            if (stillSameStep) {
                candidateHits++
            } else {
                candidate = res
                candidateHits = 1
            }
        } else {
            candidate = res
            candidateStartCell = startCell
            candidateHits = 1
        }

        if (candidateHits >= confirmCount) {
            val newBoard = candidate!!
            val moved = AssistBoard.movedSide(prev.canonical, newBoard.canonical)
            if (moved != null) {
                redGo = moved != 1
            }
            confirmed = newBoard
            candidate = null
            candidateHits = 0
            candidateStartCell = null
            unstableStreak = 0
            return Event.NEW_BOARD
        }
        unstableStreak++
        return Event.UNSTABLE
    }
}