package com.timez.chess.assist

import com.timez.chess.gamelogic.Piece
import kotlin.text.iterator

/**
 * 纯 JVM 的中国象棋棋盘抽象。
 * 约定与项目 gamelogic.Board 一致：piece[y][x]，y=0 为黑方底线，y=9 为红方底线（红在下即标准视角）。
 * 本文件不依赖任何 Android 类，便于单元测试。
 */
object AssistBoard {
    const val W = 9
    const val H = 10

    /** 标准开局 FEN（xqbase 格式，红先） */
    const val START_FEN = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1"

    /** 标准开局局面，piece[y][x]，红在 y=9 底部 */
    /**
     * 比对两个局面的"走子特征"：
     * - occupiedToEmpty：从有子变空的格子数（真实走子必为 1，即起点）
     * - emptyToOccupied：从空变有子的格子数（无吃子时为 1，吃子时为 0）
     * - typeChange：同格有子但类型变了（吃子时终点 1 格，或识别抖动）
     * 返回 intArrayOf(occupiedToEmpty, emptyToOccupied, typeChange)
     */
    fun moveDiff(a: Array<IntArray>, b: Array<IntArray>): IntArray {
        var occToEmpty = 0
        var emptyToOcc = 0
        var typeChange = 0
        for (y in 0 until H) for (x in 0 until W) {
            val pa = a[y][x]
            val pb = b[y][x]
            if (pa == pb) continue
            when {
                pa != Piece.EMPTY && pb == Piece.EMPTY -> occToEmpty++
                pa == Piece.EMPTY && pb != Piece.EMPTY -> emptyToOcc++
                else -> typeChange++
            }
        }
        return intArrayOf(occToEmpty, emptyToOcc, typeChange)
    }

    fun canonicalStart(): Array<IntArray> {
        val p = Array(H) { IntArray(W) }
        for (x in 0 until W) {
            p[0][x] = when (x) {
                0, 8 -> Piece.BJU
                1, 7 -> Piece.BMA
                2, 6 -> Piece.BXIANG
                3, 5 -> Piece.BSHI
                else -> Piece.BJIANG
            }
        }
        p[2][1] = Piece.BPAO
        p[2][7] = Piece.BPAO
        for (x in 0 until W step 2) p[3][x] = Piece.BZU
        for (x in 0 until W step 2) p[6][x] = Piece.WBING
        p[7][1] = Piece.WPAO
        p[7][7] = Piece.WPAO
        for (x in 0 until W) {
            p[9][x] = when (x) {
                0, 8 -> Piece.WJU
                1, 7 -> Piece.WMA
                2, 6 -> Piece.WXIANG
                3, 5 -> Piece.WSHI
                else -> Piece.WSHUAI
            }
        }
        return p
    }

    fun clone(p: Array<IntArray>): Array<IntArray> = Array(H) { p[it].clone() }

    fun equal(a: Array<IntArray>, b: Array<IntArray>): Boolean {
        for (y in 0 until H) for (x in 0 until W) if (a[y][x] != b[y][x]) return false
        return true
    }

    /** piece[y][x] -> xqbase FEN 盘面 + 走子方 */
    fun toFen(pieces: Array<IntArray>, redGo: Boolean): String {
        val sb = StringBuilder()
        for (y in 0 until H) {
            var zeros = 0
            for (x in 0 until W) {
                val p = pieces[y][x]
                if (p == Piece.EMPTY) zeros++
                else {
                    if (zeros > 0) { sb.append(zeros); zeros = 0 }
                    sb.append(Piece.pieceCharMap[p])
                }
            }
            if (zeros > 0) sb.append(zeros)
            if (y < H - 1) sb.append('/')
        }
        return "$sb ${if (redGo) "w" else "b"} - - 0 1"
    }

    fun piecesFromFen(fen: String): Array<IntArray> {
        val p = Array(H) { IntArray(W) }
        val board = fen.trim().split(' ')[0]
        var x = 0
        var y = 0
        for (c in board) {
            when {
                c == '/' -> { x = 0; y++ }
                c in '0'..'9' -> { x += c - '0' }
                else -> { p[y][x] = Piece.pieceValueMap[c] ?: Piece.EMPTY; x++ }
            }
        }
        return p
    }

    /**
     * 应用一个 ucci 着法（如 "h2e2"）到 FEN 局面，返回行棋方翻转后的新 FEN。
     * 着法非法或起点无子时原样返回。
     */    fun applyUcci(fen: String, ucci: String): String {
        if (ucci.length < 4) return fen
        val fx = ucci[0] - 'a'
        val fy = 9 - (ucci[1] - '0')
        val tx = ucci[2] - 'a'
        val ty = 9 - (ucci[3] - '0')
        if (fx !in 0 until W || fy !in 0 until H || tx !in 0 until W || ty !in 0 until H) return fen
        val board = piecesFromFen(fen)
        val piece = board[fy][fx]
        if (piece == Piece.EMPTY) return fen
        board[ty][tx] = piece
        board[fy][fx] = Piece.EMPTY
        val nextRedGo = fen.trim().split(' ').getOrNull(1) != "w"
        return toFen(board, nextRedGo)
    }

    /**
     * 推断"刚走子的一方"：对比旧/新局面。返回红(1)、黑(0)或 null(无法确定)。
     * 规则：新增棋子（目标格）所属阵营即移动方；无新增（重排/异常）则尝试用移除方补。
     */
    fun movedSide(old: Array<IntArray>, new: Array<IntArray>): Int? {
        var addedSide: Int? = null
        var removedSide: Int? = null
        for (y in 0 until H) for (x in 0 until W) {
            val o = old[y][x]
            val n = new[y][x]
            if (o == n) continue
            if (n != Piece.EMPTY) {
                val s = if (Piece.isRed(n)) 1 else 0
                if (addedSide != null && addedSide != s) return null
                addedSide = s
            }
            if (o != Piece.EMPTY) {
                val s = if (Piece.isRed(o)) 1 else 0
                if (removedSide != null && removedSide != s) return null
                removedSide = s
            }
        }
        return addedSide ?: removedSide
    }

    /**
     * 宽容合法性校验：只拦"严重异常"帧（缺帅/将、总子数超限、同方帅>2）。
     * 棋子数量上限不再严格限制——YOLO 中局难免把车/马混淆，严格校验会让
     * 90% 的正常帧被拒，导致"识别不稳定"。
     */
    fun validate(pieces: Array<IntArray>): List<String> {
        val issues = mutableListOf<String>()
        val counts = IntArray(Piece.BZU + 1)
        for (y in 0 until H) for (x in 0 until W) {
            val p = pieces[y][x]
            if (p != Piece.EMPTY) counts[p]++
        }
        // 只拦真正离谱的：帅/将必须存在，且不能有 3 个以上
        val redKing = counts[Piece.WSHUAI]
        val blackKing = counts[Piece.BJIANG]
        if (redKing < 1) issues.add("红帅缺失")
        if (blackKing < 1) issues.add("黑将缺失")
        if (redKing > 2) issues.add("红帅过多:$redKing")
        if (blackKing > 2) issues.add("黑将过多:$blackKing")

        var total = 0
        for (c in counts) total += c
        if (total > 32) issues.add("总子数超限:$total")

        // 红/黑任一方棋子总数超过 16（初始上限），说明识别把对方棋子算到了本方
        var redTotal = 0
        var blackTotal = 0
        for (y in 0 until H) for (x in 0 until W) {
            val p = pieces[y][x]
            if (p == Piece.EMPTY) continue
            if (Piece.isRed(p)) redTotal++ else blackTotal++
        }
        if (redTotal > 16) issues.add("红方子数超限:$redTotal")
        if (blackTotal > 16) issues.add("黑方子数超限:$blackTotal")

        return issues
    }

    /** 与开局对照一致的子数（用于"开局检测"）；开局局面只读缓存，避免每帧重建 */
    private val startBoard = canonicalStart()

    fun matchStartCount(pieces: Array<IntArray>): Int {
        var n = 0
        for (y in 0 until H) for (x in 0 until W) {
            if (pieces[y][x] != Piece.EMPTY && pieces[y][x] == startBoard[y][x]) n++
        }
        return n
    }
}
