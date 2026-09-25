package com.timez.chess.assist.ui

import android.app.Dialog
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.Button
import android.widget.EditText
import android.widget.ProgressBar
import android.widget.TextView
import androidx.fragment.app.DialogFragment
import com.timez.chess.R
import com.timez.chess.assist.AssistConfig
import com.timez.chess.assist.RemoteEngineTester

/**
 * 远程对弈配置对话框：输入 IP/端口 → 测试连接 → 成功后回调 onConnected。
 */
class RemoteEngineDialog : DialogFragment() {

    /** 连接成功回调（在 UI 线程） */
    var onConnected: (() -> Unit)? = null

    private lateinit var config: AssistConfig
    private lateinit var etHost: EditText
    private lateinit var etPort: EditText
    private lateinit var tvError: TextView
    private lateinit var btnConnect: Button
    private lateinit var progress: ProgressBar

    private val mainHandler = Handler(Looper.getMainLooper())

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = super.onCreateDialog(savedInstanceState)
        dialog.setCanceledOnTouchOutside(false)
        return dialog
    }


    override fun onStart() {
        super.onStart()
        dialog?.window?.let { window ->
            val dm = resources.displayMetrics
            val width = (dm.widthPixels * 0.88f).toInt()
            window.setLayout(width, WindowManager.LayoutParams.WRAP_CONTENT)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ): View {
        config = AssistConfig(requireContext())
        val root = inflater.inflate(R.layout.dialog_remote_engine, container, false)
        etHost = root.findViewById(R.id.et_host)
        etPort = root.findViewById(R.id.et_port)
        tvError = root.findViewById(R.id.tv_error)
        btnConnect = root.findViewById(R.id.btn_connect)
        progress = root.findViewById(R.id.progress)

        etHost.setText(config.remoteHost)
        etPort.setText(if (config.remotePort > 0) config.remotePort.toString() else "10000")

        btnConnect.setOnClickListener { tryConnect() }
        root.findViewById<Button>(R.id.btn_cancel).setOnClickListener { dismiss() }
        return root
    }

    private fun tryConnect() {
        val host = etHost.text.toString().trim()
        val port = etPort.text.toString().trim().toIntOrNull() ?: 0

        if (host.isBlank()) { showError("请填写电脑 IP 地址"); return }
        if (port !in 1..65535) { showError("端口号无效（应为 1–65535）"); return }

        setLoading(true)
        tvError.visibility = View.GONE

        Thread {
            val err = RemoteEngineTester.test(host, port)
            mainHandler.post {
                setLoading(false)
                if (err == null) {
                    config.engineMode = "remote"
                    config.remoteHost = host
                    config.remotePort = port
                    onConnected?.invoke()
                    dismiss()
                } else {
                    showError(err)
                }
            }
        }.start()
    }

    private fun setLoading(loading: Boolean) {
        progress.visibility = if (loading) View.VISIBLE else View.GONE
        btnConnect.isEnabled = !loading
        etHost.isEnabled = !loading
        etPort.isEnabled = !loading
        btnConnect.text = if (loading) "连接中…" else "连接"
    }

    private fun showError(msg: String) {
        tvError.text = msg
        tvError.visibility = View.VISIBLE
    }
}