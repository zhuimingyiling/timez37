package com.timez.chess.assist

import android.util.Log
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.ConnectException
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketTimeoutException
import java.net.UnknownHostException

/**
 * 远程引擎连接测试：TCP 连接 + UCI 握手（uci/uciok + isready/readyok）。
 * 成功返回 null，失败返回错误消息。
 */
object RemoteEngineTester {

    private const val TAG = "RemoteEngineTester"

    /**
     * 测试远程引擎是否可用。
     * @return null 表示成功；非 null 为错误提示文本
     */
    fun test(host: String, port: Int, timeoutMs: Int = 5000): String? {
        if (host.isBlank()) return "请填写电脑 IP 地址"
        if (port !in 1..65535) return "端口号无效（应为 1–65535）"

        var socket: Socket? = null
        var reader: BufferedReader? = null
        var writer: BufferedWriter? = null
        return try {
            socket = Socket().apply {
                connect(InetSocketAddress(host, port), timeoutMs)
                soTimeout = timeoutMs
                tcpNoDelay = true
            }
            reader = BufferedReader(InputStreamReader(socket.getInputStream()))
            writer = BufferedWriter(OutputStreamWriter(socket.getOutputStream()))

            // UCI 握手
            writer.write("uci\n"); writer.flush()
            if (!waitFor(reader, "uciok", timeoutMs)) {
                return "引擎无响应（未收到 uciok），请确认电脑端 RemoteUCI 已启动"
            }

            writer.write("isready\n"); writer.flush()
            if (!waitFor(reader, "readyok", timeoutMs)) {
                return "引擎未就绪（未收到 readyok），请检查电脑端皮卡鱼是否正常加载"
            }
            null
        } catch (e: SocketTimeoutException) {
            "连接超时：请检查 IP/端口是否正确、手机与电脑是否在同一 Wi-Fi"
        } catch (e: ConnectException) {
            "连接被拒绝：请确认电脑端 RemoteUCI 正在运行，且防火墙已放行端口 $port"
        } catch (e: UnknownHostException) {
            "无法解析主机名：请检查 IP 地址是否填写正确"
        } catch (e: Exception) {
            Log.e(TAG, "test failed", e)
            "连接失败：${e.message ?: "未知错误"}"
        } finally {
            try { reader?.close() } catch (_: Exception) {}
            try { writer?.close() } catch (_: Exception) {}
            try { socket?.close() } catch (_: Exception) {}
        }
    }

    private fun waitFor(reader: BufferedReader, token: String, timeoutMs: Int): Boolean {
        val deadline = System.currentTimeMillis() + timeoutMs
        while (System.currentTimeMillis() < deadline) {
            val line = reader.readLine() ?: return false
            if (line.contains(token)) return true
        }
        return false
    }
}