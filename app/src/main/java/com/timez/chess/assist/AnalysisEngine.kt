package com.timez.chess.assist

import android.content.Context
import android.util.Log
import org.petero.droidfish.engine.EngineConfig
import org.petero.droidfish.engine.UCIEngine
import org.petero.droidfish.engine.UCIEngineBase
import org.petero.droidfish.player.EngineListener

import java.util.concurrent.Executors

/**
 * 分析引擎：直接驱动一个独立 pikafish UCI 进程（与游戏内引擎进程互不干扰）。
 */
class AnalysisEngine(
    private val context: Context,
    private val listener: Listener,
    searchDepth: Int = 20,
    private val maxPv: Int = 3,
    initialThreads: Int = Runtime.getRuntime().availableProcessors(),
) : AssistEngine {

    interface Listener {
        fun onEngineReady()
        fun onSearchUpdate(result: AnalysisResult)
        fun onSearchDone(result: AnalysisResult)
        fun onEngineError(message: String)
    }

    private val tag = "AnalysisEngine"
    private val executor = Executors.newSingleThreadExecutor { r ->
        Thread(r, "analysis-engine").apply { isDaemon = true }
    }
    private val engineListener = object : EngineListener {
        override fun reportEngineError(errMsg: String?) {
            Log.w(tag, "engine error: $errMsg")
            listener.onEngineError(errMsg ?: "引擎错误")
        }
        override fun notifyEngineName(engineName: String?) {}
        override fun notifySearchResult(searchId: Int, bestMove: String?, nextPonderMove: String?) {}
        override fun notifyEvalResult(searchId: Int, eval: Float) {}
        override fun notifyEngineInitialized() {}
    }

    @Volatile private var engine: UCIEngine? = null
    @Volatile private var ready = false
    @Volatile private var started = false

    @Volatile private var pendingFen: String? = null
    private val lock = Object()

    @Volatile private var searchDepth: Int = searchDepth.coerceIn(6, 40)
    @Volatile private var hashMb: Int = 256
    @Volatile private var appliedHash = -1
    @Volatile private var threads: Int = initialThreads.coerceIn(1, 128)
    @Volatile private var appliedThreads = -1

    override fun setSearchDepth(depth: Int) {
        searchDepth = depth.coerceIn(6, 40)
    }

    override fun setHash(mb: Int) {
        hashMb = mb
    }

    override fun setThreads(n: Int) {
        threads = n.coerceIn(1, 128)
    }

    override fun start() {
        synchronized(lock) {
            if (started) return
            started = true
        }
        executor.execute {
            var attempt = 0
            while (!ready && !shuttingDown && attempt < 2) {
                attempt++
                try {
                    doStart()
                } catch (e: Exception) {
                    Log.e(tag, "engine start failed (attempt $attempt)", e)
                    if (attempt >= 2) {
                        listener.onEngineError("引擎启动失败：${e.message ?: "未知原因"}（请重新连线重试）")
                    }
                }
                if (!ready && !shuttingDown && attempt < 2) {
                    runCatching { engine?.shutDown() }
                    engine = null
                    try { Thread.sleep(1200) } catch (e: InterruptedException) { return@execute }
                }
            }
            if (!ready) return@execute

            while (!shuttingDown) {
                val fen = pendingFen
                if (fen == null) {
                    synchronized(lock) {
                        try { lock.wait(300) } catch (e: InterruptedException) {
                            Thread.currentThread().interrupt(); return@execute
                        }
                    }
                    continue
                }
                pendingFen = null
                searchOnce(fen)
            }
        }
    }

    private fun doStart() {
        val ec = EngineConfig().apply { workDir = context.filesDir.absolutePath }
        val eng: UCIEngine = UCIEngineBase.getEngine("pikafish", ec, engineListener)
            ?: throw IllegalStateException("无法创建 pikafish 引擎")
        engine = eng
        eng.initialize()

        eng.writeLineToEngine("uci")

        var uciok = false
        var elapsed = 0L
        val dl = 200L
        while (!uciok && elapsed < 10000) {
            val line = eng.readLineFromEngine(dl.toInt()) ?: throw IllegalStateException("引擎进程意外退出")
            elapsed += dl
            if (line.isEmpty()) continue
            val tokens = line.trim().split(Regex("\\s+"))
            when {
                tokens[0] == "uciok" -> uciok = true
                tokens[0] == "id" || tokens[0] == "option" -> eng.registerOption(tokens.toTypedArray())
            }
        }
        if (!uciok) throw IllegalStateException("引擎握手超时（uciok），多为 CPU/兼容性问题")

        eng.initConfig(ec)

        eng.setOption("EvalFile", "libpikafish.nnue.so")
        eng.setOption("Threads", threads)
        appliedThreads = threads
        eng.setOption("Hash", hashMb)
        appliedHash = hashMb
        eng.setOption("MultiPV", maxPv)

        eng.writeLineToEngine("ucinewgame")
        eng.writeLineToEngine("isready")
        var readyOk = false
        elapsed = 0L
        while (!readyOk && elapsed < 15000) {
            val line = eng.readLineFromEngine(dl.toInt()) ?: throw IllegalStateException("引擎进程意外退出")
            elapsed += dl
            if (line.trim() == "readyok") readyOk = true
        }
        if (!readyOk) throw IllegalStateException("引擎就绪握手失败（readyok）")
        ready = true
        Log.i(tag, "analysis engine ready, threads=$threads hash=$hashMb")
        listener.onEngineReady()
    }

    @Volatile private var shuttingDown = false

    private fun searchOnce(fen: String) {
        val eng = engine ?: return
        val lines = LinkedHashMap<Int, MutableAnalysisLine>()
        if (appliedHash != hashMb) {
            eng.setOption("Hash", hashMb)
            appliedHash = hashMb
        }
        if (appliedThreads != threads) {
            eng.setOption("Threads", threads)
            appliedThreads = threads
        }
        eng.writeLineToEngine("position fen $fen")
        eng.writeLineToEngine("go depth $searchDepth")

        val deadline = System.currentTimeMillis() + 60000
        while (System.currentTimeMillis() < deadline) {
            val line = eng.readLineFromEngine(250) ?: return
            val tokens = line.trim().split(Regex("\\s+"))
            if (tokens.isEmpty()) continue
            if (tokens[0] == "info") {
                if (tokens.contains("string")) Log.i(tag, "engine: ${line.trim()}")
                parseInfo(tokens)?.let { ml ->
                    lines[ml.multiPv] = ml
                    val redGo = try { fen.split(" ")[1] != "b" } catch (e: Exception) { true }
                    listener.onSearchUpdate(AnalysisResult(fen, redGo, null,
                        lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }))
                }
            } else if (tokens[0] == "bestmove") {
                val bm = if (tokens.size > 1) tokens[1] else null
                val redGo = try { fen.split(" ")[1] != "b" } catch (e: Exception) { true }
                listener.onSearchDone(AnalysisResult(fen, redGo, bm,
                    lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }))
                return
            }
        }
        eng.writeLineToEngine("stop")
        val dl2 = System.currentTimeMillis() + 1500
        while (System.currentTimeMillis() < dl2) {
            val line = eng.readLineFromEngine(250) ?: return
            val tokens = line.trim().split(Regex("\\s+"))
            if (tokens.isEmpty()) continue
            if (tokens[0] == "bestmove") {
                val bm = if (tokens.size > 1) tokens[1] else null
                val redGo = try { fen.split(" ")[1] != "b" } catch (e: Exception) { true }
                listener.onSearchDone(AnalysisResult(fen, redGo, bm,
                    lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }))
                return
            }
        }
        listener.onSearchDone(AnalysisResult(fen, true, null,
            lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }))
    }

    private class MutableAnalysisLine(val multiPv: Int) {
        var depth = 0
        var scoreCp = 0
        var mateIn: Int? = null
        var pv: List<String> = emptyList()
        fun toAnalysisLine(): AnalysisLine = AnalysisLine(multiPv, depth, scoreCp, mateIn, pv)
    }

    private fun parseInfo(tokens: List<String>): MutableAnalysisLine? {
        var multiPv = 1
        var depth = 0
        var scoreCp: Int? = null
        var mateIn: Int? = null
        var pv: List<String> = emptyList()
        var i = 1
        while (i < tokens.size) {
            when (tokens[i]) {
                "depth" -> if (i + 1 < tokens.size) { depth = tokens[i + 1].toIntOrNull() ?: depth; i++ }
                "multipv" -> if (i + 1 < tokens.size) { multiPv = tokens[i + 1].toIntOrNull() ?: multiPv; i++ }
                "score" -> if (i + 2 < tokens.size) {
                    when (tokens[i + 1]) {
                        "cp" -> { scoreCp = tokens[i + 2].toIntOrNull() ?: 0; i += 2 }
                        "mate" -> { mateIn = tokens[i + 2].toIntOrNull(); scoreCp = 0; i += 2 }
                    }
                }
                "pv" -> { pv = tokens.subList(i + 1, tokens.size).filter { it.length == 4 }; i = tokens.size }
                "currmove" -> i = tokens.size
            }
            i++
        }
        if (scoreCp == null && mateIn == null) return null
        return MutableAnalysisLine(multiPv).apply {
            this.depth = depth
            this.scoreCp = scoreCp ?: 0
            this.mateIn = mateIn
            this.pv = pv
        }
    }

    override fun request(fen: String) {
        pendingFen = fen
        val eng = engine
        if (eng != null && ready) {
            eng.writeLineToEngine("stop")
        }
        synchronized(lock) { lock.notifyAll() }
    }

    override fun shutdown() {
        shuttingDown = true
        ready = false
        synchronized(lock) { lock.notifyAll() }
        try { engine?.shutDown() } catch (e: Exception) { Log.w(tag, "shutDown failed", e) }
        engine = null
        executor.shutdownNow()
    }
}