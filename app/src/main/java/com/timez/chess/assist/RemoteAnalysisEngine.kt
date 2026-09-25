package com.timez.chess.assist

import android.util.Log

import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.IOException
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketTimeoutException
import java.util.concurrent.Executors

/**
 * 远程分析引擎：通过 TCP 连接到电脑端的 RemoteUCI + Pikafish。
 * Listener 复用 [AnalysisEngine.Listener]，上层 UI 无需任何改动。
 */
class RemoteAnalysisEngine(
    private val host: String,
    private val port: Int,
    private val listener: AnalysisEngine.Listener,
    initialDepth: Int = 20,
    private val maxPv: Int = 3,
    initialThreads: Int = Runtime.getRuntime().availableProcessors(),
) : AssistEngine {

    private val tag = "RemoteAnalysisEngine"
    private val executor = Executors.newSingleThreadExecutor { r ->
        Thread(r, "remote-analysis-engine").apply { isDaemon = true }
    }

    @Volatile private var socket: Socket? = null
    @Volatile private var reader: BufferedReader? = null
    @Volatile private var writer: BufferedWriter? = null
    private val writeLock = Object()

    @Volatile private var ready = false
    @Volatile private var started = false
    @Volatile private var shuttingDown = false

    @Volatile private var pendingFen: String? = null
    private val lock = Object()

    @Volatile private var searchDepth: Int = initialDepth.coerceIn(6, 40)
    @Volatile private var hashMb: Int = 256
    @Volatile private var appliedHash = -1
    @Volatile private var threads: Int = initialThreads.coerceIn(1, 128)
    @Volatile private var appliedThreads = -1

    override fun setSearchDepth(depth: Int) {
        searchDepth = depth.coerceIn(6, 40)
    }

    override fun setHash(mb: Int) {
        hashMb = mb
        if (ready) {
            sendCommand("setoption name Hash value $mb")
            appliedHash = mb
        }
    }

    override fun setThreads(n: Int) {
        threads = n.coerceIn(1, 128)
        if (ready) {
            sendCommand("setoption name Threads value $threads")
            appliedThreads = threads
        }
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
                    Log.e(tag, "remote engine start failed (attempt $attempt)", e)
                    if (attempt >= 2) {
                        listener.onEngineError("远程引擎连接失败：${e.message ?: "未知原因"}")
                    }
                }
                if (!ready && !shuttingDown && attempt < 2) {
                    runCatching { closeSocket() }
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
                try {
                    searchOnce(fen)
                } catch (e: Exception) {
                    Log.e(tag, "searchOnce failed", e)
                    listener.onEngineError("远程分析中断：${e.message ?: "连接断开"}")
                    ready = false
                    runCatching { closeSocket() }
                    try {
                        doStart()
                    } catch (ex: Exception) {
                        Log.e(tag, "reconnect failed", ex)
                        listener.onEngineError("远程引擎重连失败，请检查电脑端 RemoteUCI 是否在运行")
                        return@execute
                    }
                }
            }
        }
    }

    private fun doStart() {
        closeSocket()
        val s = Socket()
        try {
            s.connect(InetSocketAddress(host, port), 5000)
        } catch (e: IOException) {
            s.close()
            throw IllegalStateException("无法连接 $host:$port（${e.message}）", e)
        }
        s.tcpNoDelay = true
        s.soTimeout = 30000
        socket = s
        reader = BufferedReader(InputStreamReader(s.getInputStream()))
        writer = BufferedWriter(OutputStreamWriter(s.getOutputStream()))

        sendCommand("uci")
        if (!waitForToken("uciok", 10000)) throw IllegalStateException("引擎握手超时（uciok）")

        sendCommand("setoption name Threads value $threads")
        appliedThreads = threads
        sendCommand("setoption name Hash value $hashMb")
        appliedHash = hashMb
        sendCommand("setoption name MultiPV value $maxPv")

        sendCommand("ucinewgame")
        sendCommand("isready")
        if (!waitForToken("readyok", 15000)) throw IllegalStateException("引擎就绪握手失败（readyok）")

        ready = true
        Log.i(tag, "remote analysis engine ready at $host:$port threads=$threads hash=$hashMb")
        listener.onEngineReady()
    }

    private fun waitForToken(token: String, timeoutMs: Long): Boolean {
        val r = reader ?: return false
        val deadline = System.currentTimeMillis() + timeoutMs
        while (System.currentTimeMillis() < deadline && !shuttingDown) {
            val line = try {
                r.readLine()
            } catch (e: SocketTimeoutException) {
                continue
            } catch (e: IOException) {
                return false
            }
            if (line == null) return false
            if (line.contains(token)) return true
        }
        return false
    }

    private fun searchOnce(fen: String) {
        val r = reader ?: throw IOException("reader is null")
        val lines = LinkedHashMap<Int, MutableAnalysisLine>()

        if (appliedHash != hashMb) {
            sendCommand("setoption name Hash value $hashMb")
            appliedHash = hashMb
        }
        if (appliedThreads != threads) {
            sendCommand("setoption name Threads value $threads")
            appliedThreads = threads
        }
        sendCommand("position fen $fen")
        sendCommand("go depth $searchDepth")

        val deadline = System.currentTimeMillis() + 60000
        while (System.currentTimeMillis() < deadline && !shuttingDown) {
            val line: String? = try {
                r.readLine()
            } catch (e: SocketTimeoutException) {
                continue
            }
            if (line == null) throw IOException("远程引擎连接已断开")

            val tokens = line.trim().split(Regex("\\s+"))
            if (tokens.isEmpty()) continue

            when (tokens[0]) {
                "info" -> {
                    if (tokens.contains("string")) Log.i(tag, "engine: ${line.trim()}")
                    parseInfo(tokens)?.let { ml ->
                        lines[ml.multiPv] = ml
                        val redGo = try { fen.split(" ")[1] != "b" } catch (e: Exception) { true }
                        listener.onSearchUpdate(
                            AnalysisResult(
                                fen, redGo, null,
                                lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }
                            )
                        )
                    }
                }
                "bestmove" -> {
                    val bm = if (tokens.size > 1) tokens[1] else null
                    val redGo = try { fen.split(" ")[1] != "b" } catch (e: Exception) { true }
                    listener.onSearchDone(
                        AnalysisResult(
                            fen, redGo, bm,
                            lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }
                        )
                    )
                    return
                }
            }
        }

        sendCommand("stop")
        val dl2 = System.currentTimeMillis() + 1500
        while (System.currentTimeMillis() < dl2) {
            val line = try {
                r.readLine()
            } catch (e: SocketTimeoutException) {
                continue
            } ?: return
            val tokens = line.trim().split(Regex("\\s+"))
            if (tokens.isEmpty()) continue
            if (tokens[0] == "bestmove") {
                val bm = if (tokens.size > 1) tokens[1] else null
                val redGo = try { fen.split(" ")[1] != "b" } catch (e: Exception) { true }
                listener.onSearchDone(
                    AnalysisResult(
                        fen, redGo, bm,
                        lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }
                    )
                )
                return
            }
        }
        listener.onSearchDone(
            AnalysisResult(
                fen, true, null,
                lines.values.sortedBy { it.multiPv }.map { it.toAnalysisLine() }
            )
        )
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
                "score" -> {
                    if (i + 2 < tokens.size) {
                        when (tokens[i + 1]) {
                            "cp" -> { scoreCp = tokens[i + 2].toIntOrNull() ?: 0; i += 2 }
                            "mate" -> { mateIn = tokens[i + 2].toIntOrNull(); scoreCp = 0; i += 2 }
                        }
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
        if (ready) sendCommand("stop")
        synchronized(lock) { lock.notifyAll() }
    }

    override fun shutdown() {
        shuttingDown = true
        ready = false
        synchronized(lock) { lock.notifyAll() }
        try { sendCommand("quit") } catch (e: Exception) { }
        closeSocket()
        executor.shutdownNow()
    }

    private fun sendCommand(cmd: String) {
        val w = writer ?: return
        synchronized(writeLock) {
            try {
                w.write(cmd)
                w.write("\n")
                w.flush()
            } catch (e: IOException) {
                Log.e(tag, "sendCommand failed: $cmd", e)
            }
        }
    }

    private fun closeSocket() {
        try { reader?.close() } catch (e: Exception) { }
        try { writer?.close() } catch (e: Exception) { }
        try { socket?.close() } catch (e: Exception) { }
        reader = null
        writer = null
        socket = null
    }
}