package com.timez.chess.assist

/** 引擎统一接口：本地 [AnalysisEngine] 与远程 [RemoteAnalysisEngine] 都实现它。 */
interface AssistEngine {
    fun start()
    fun request(fen: String)
    fun setSearchDepth(depth: Int)
    fun setHash(mb: Int)
    fun setThreads(n: Int)
    fun shutdown()
}