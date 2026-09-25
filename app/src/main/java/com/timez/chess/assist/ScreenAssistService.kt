package com.timez.chess.assist

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.Image
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Binder
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.Looper
import android.provider.Settings
import android.util.DisplayMetrics
import android.util.Log
import android.view.Gravity
import android.view.WindowManager
import androidx.core.app.ServiceCompat
import com.timez.chess.assist.ui.AssistActivity
import com.timez.chess.R
import com.timez.chess.assist.ui.OverlayAction
import com.timez.chess.assist.ui.OverlayModel
import com.timez.chess.assist.ui.OverlayPanelView
import com.timez.chess.gamelogic.Piece
import com.timez.chess.gamelogic.Zobrist
import com.timez.chess.openbook.BHOpenBook
import com.timez.chess.openbook.BookData
import com.timez.chess.openbook.OpenBook
import com.timez.chess.BuildConfig
import java.io.DataOutputStream
import java.io.File
import java.io.FileOutputStream
import kotlin.math.max
import kotlin.math.min

class ScreenAssistService : Service() {

    companion object {
        private const val TAG = "AssistService"
        const val ACTION_START = "com.timez.chess.assist.START"
        const val EXTRA_RESULT_CODE = "result_code"
        const val EXTRA_RESULT_DATA = "result_data"
        private const val CHANNEL_ID = "assist_foreground"
        private const val NOTIF_ID = 1001
        private const val MIN_FRAME_INTERVAL_MS = 120L
        private const val OVERLAY_FRAME_INTERVAL_MS = 1400L
        private const val MIN_RECOGNIZE_INTERVAL_MS = 150L
        private const val SUGGEST_STABLE_MS = 1200L
        const val MIN_RECOGNIZED_PIECES = 3
    }

    inner class CastBinder : Binder() {
        fun service(): ScreenAssistService = this@ScreenAssistService
    }

    fun isCapturing(): Boolean = mediaProjection != null && virtualDisplay != null

    fun getStatusText(): String = lastStatusText

    /** 手机端 UI 调用：热更新引擎线程数（本地/远程都生效） */
    fun setEngineThreads(n: Int) {
        config.threads = n
        analysisEngine?.setThreads(n)
    }

    private val config by lazy { AssistConfig(this) }

    private var mediaProjection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null
    private val projectionManager by lazy {
        getSystemService(MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
    }

    private val workerThread = HandlerThread("assist-worker").apply { start() }
    private val workerHandler = Handler(workerThread.looper)

    private val frameThread = HandlerThread("assist-frame").apply { start() }
    private val frameHandler = Handler(frameThread.looper)
    private val mainHandler = Handler(Looper.getMainLooper())

    private var lastFrameTs = 0L
    private var lastRecognizeTs = 0L
    private var pendingFrame = false

    @Volatile private var paused = false
    @Volatile private var manualMode = false
    @Volatile private var manualSelected: Pair<Int, Int>? = null
    @Volatile private var overlayClosed = false
    @Volatile private var userDragging = false

    private var tracker = BoardTracker(3)
    private var yoloDetector: YoloBoardDetector? = null

    @Volatile private var yoloMissStreak = 0
    @Volatile private var cropUnstableStreak = 0

    @Volatile private var sessionGrid: BoardGrid? = null

    private var analysisEngine: AssistEngine? = null
    @Volatile private var engineReady = false
    @Volatile private var engineErrorText: String? = null

    @Volatile private var stableUcci: String? = null
    @Volatile private var stableSince = 0L
    @Volatile private var searchSettled = false

    @Volatile private var currentFen = ""
    @Volatile private var currentPieces: IntArray? = null
    @Volatile private var currentCanonical: Array<IntArray>? = null
    private var currentOrientation = Orientation.STANDARD
    private var lastAnalysis: AnalysisResult? = null
    private var selectedCandidate = 0

    private var bookMoves: List<BookData>? = null
    private var openBook: BHOpenBook? = null
    private var openBookFailed = false

    private class HistoryEntry(val result: RecognitionResult, val redGo: Boolean, val fen: String)
    private val history = ArrayDeque<HistoryEntry>()

    private val strengthLevels = listOf("快速·深度12" to 12, "标准·深度20" to 20, "强劲·深度24" to 24)
    private var strengthIndex = 0
    private val hashLevels = listOf(256, 512, 1024, 2048)
    private var hashIndex = 0

    private var windowManager: WindowManager? = null
    private var overlayPanel: OverlayPanelView? = null
    private var overlayParams: WindowManager.LayoutParams? = null

    private val binder = CastBinder()

    override fun onCreate() {
        super.onCreate()
        tracker = BoardTracker(config.confirmCount)
        val idx = strengthLevels.indexOfFirst { it.second == config.searchDepth }
        strengthIndex = if (idx >= 0) idx else 1
        val hidx = hashLevels.indexOfFirst { it == config.hashMb }
        hashIndex = if (hidx >= 0) hidx else 0
        yoloDetector = try {
            YoloBoardDetector(this)
        } catch (t: Throwable) {
            Log.w(TAG, "yolo detector init failed", t)
            null
        }
        Log.i(TAG, "yolo detector: ${if (yoloDetector != null) "ready" else "unavailable"}")
    }

    override fun onBind(intent: Intent?): IBinder = binder

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_START) {
            val resultCode = intent.getIntExtra(EXTRA_RESULT_CODE, 0)
            @Suppress("DEPRECATION")
            val data = if (Build.VERSION.SDK_INT >= 33)
                intent.getParcelableExtra(EXTRA_RESULT_DATA, Intent::class.java)
            else intent.getParcelableExtra(EXTRA_RESULT_DATA)
            startForegroundInternal()
            startCapture(resultCode, data)
        }
        return START_STICKY
    }

    private fun startForegroundInternal() {
        createChannel()
        val notification = buildNotification()
        val type = if (Build.VERSION.SDK_INT >= 29)
            ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION else 0
        ServiceCompat.startForeground(this, NOTIF_ID, notification, type)
    }

    private fun createChannel() {
        val nm = getSystemService(NotificationManager::class.java)
        val ch = NotificationChannel(CHANNEL_ID, "连线·屏幕识别", NotificationManager.IMPORTANCE_LOW)
        ch.description = "屏幕识别与悬浮窗指导运行中"
        nm.createNotificationChannel(ch)
    }

    private fun buildNotification(contentText: String? = null): Notification {
        val pi = PendingIntent.getActivity(
            this, 0,
            Intent(this, AssistActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE
        )
        return Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("timez37·连线识别运行中")
            .setContentText(contentText ?: "仅识别局面并给出走法建议，不会自动走子")
            .setSmallIcon(R.drawable.ic_launcher)
            .setOngoing(true)
            .setContentIntent(pi)
            .build()
    }

    // ==================== 截屏 ====================

    private fun startCapture(resultCode: Int, data: Intent?) {
        if (mediaProjection != null || data == null) return
        overlayClosed = false
        paused = false
        val metrics = screenMetrics()
        val w = metrics.widthPixels
        val h = metrics.heightPixels
        val dpi = metrics.densityDpi

        mediaProjection = projectionManager.getMediaProjection(resultCode, data)
        config.frameSize = w to h
        val reader = ImageReader.newInstance(w, h, PixelFormat.RGBA_8888, 2)
        reader.setOnImageAvailableListener(::onImageAvailable, frameHandler)
        imageReader = reader
        virtualDisplay = mediaProjection?.createVirtualDisplay(
            "assist-capture", w, h, dpi,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            reader.surface, null, workerHandler
        )
        Log.i(TAG, "capture started: ${w}x$h")

        val engineMode = config.engineMode
        analysisEngine = when (engineMode) {
            "remote" -> RemoteAnalysisEngine(
                host = config.remoteHost,
                port = config.remotePort,
                listener = analysisListener,
                initialDepth = config.searchDepth,
                initialThreads = config.threads
            )
            else -> AnalysisEngine(
                this, analysisListener, config.searchDepth,
                initialThreads = config.threads
            )
        }
        analysisEngine?.setHash(config.hashMb)
        analysisEngine?.start()
        postRender()
    }

    @Suppress("DEPRECATION")
    private fun screenMetrics(): DisplayMetrics {
        val dm = DisplayMetrics()
        val wm = getSystemService(WINDOW_SERVICE) as WindowManager
        @Suppress("DEPRECATION")
        wm.defaultDisplay.getRealMetrics(dm)
        return dm
    }

    private fun onImageAvailable(reader: ImageReader) {
        val now = System.currentTimeMillis()
        val slowPath = !userDragging && overlayIntersectsBoard()
        val interval = if (slowPath) OVERLAY_FRAME_INTERVAL_MS else MIN_FRAME_INTERVAL_MS
        if (now - lastFrameTs < interval) {
            reader.acquireLatestImage()?.close()
            return
        }
        lastFrameTs = now
        var needRestoreHide = false
        try {
            val needHide = slowPath && (overlayPanel?.alpha ?: 0f) > 0f
            if (needHide) {
                mainHandler.post { overlayPanel?.alpha = 0f }
                needRestoreHide = true
                Thread.sleep(60)
            }
            val image = reader.acquireLatestImage() ?: return
            try {
                val frame = imageToFrame(image)
                if (!pendingFrame) {
                    pendingFrame = true
                    workerHandler.post { pendingFrame = false; handleFrame(frame) }
                }
            } finally {
                image.close()
            }
        } catch (e: Exception) {
            Log.w(TAG, "onImageAvailable failed", e)
        } finally {
            if (needRestoreHide) {
                mainHandler.post { overlayPanel?.alpha = 1f }
            }
        }
    }

    private var frameRowBytes: ByteArray? = null

    private fun imageToFrame(image: Image): Frame {
        val plane = image.planes[0]
        val rowStride = plane.rowStride
        val pixelStride = plane.pixelStride
        val buffer = plane.buffer
        val w = image.width
        val h = image.height
        val px = IntArray(w * h)
        if (pixelStride == 4) {
            var row = frameRowBytes
            if (row == null || row.size < w * 4) row = ByteArray(w * 4)
            frameRowBytes = row
            var p = 0
            for (y in 0 until h) {
                buffer.position(y * rowStride)
                buffer.get(row, 0, w * 4)
                var i = 0
                while (i < w * 4) {
                    px[p++] = (0xFF shl 24) or
                            ((row[i].toInt() and 0xFF) shl 16) or
                            ((row[i + 1].toInt() and 0xFF) shl 8) or
                            (row[i + 2].toInt() and 0xFF)
                    i += 4
                }
            }
        } else {
            for (y in 0 until h) {
                val rowStart = y * rowStride
                for (x in 0 until w) {
                    val off = rowStart + x * pixelStride
                    val r = buffer.get(off).toInt() and 0xFF
                    val g = buffer.get(off + 1).toInt() and 0xFF
                    val b = buffer.get(off + 2).toInt() and 0xFF
                    px[y * w + x] = (0xFF shl 24) or (r shl 16) or (g shl 8) or b
                }
            }
        }
        return Frame(w, h, px)
    }

    private fun debugSaveFrameIfRequested(frame: Frame) {
        if (!BuildConfig.DEBUG) return
        val trigger = File("/sdcard/assist_debug_save")
        if (!trigger.exists()) return
        trigger.delete()
        try {
            val dir = getExternalFilesDir(null) ?: filesDir
            val outPath = File(dir, "assist_frame.raw").absolutePath
            DataOutputStream(FileOutputStream(outPath)).use { out ->
                out.writeInt(0x41534652)
                out.writeInt(frame.width)
                out.writeInt(frame.height)
                for (p in frame.argb) {
                    out.writeByte((p shr 16) and 0xFF)
                    out.writeByte((p shr 8) and 0xFF)
                    out.writeByte(p and 0xFF)
                }
            }
            Log.i(TAG, "debug frame saved $outPath")
        } catch (e: Exception) {
            Log.w(TAG, "debug frame save failed", e)
        }
    }

    // ==================== 识别与分析 ====================

    private fun cropHint(frame: Frame): DoubleArray? {
        val g = sessionGrid ?: return null
        val x0 = g.nx0 * frame.width
        val y0 = g.ny0 * frame.height
        val x1 = g.nx1 * frame.width
        val y1 = g.ny1 * frame.height
        val mx = (x1 - x0) / 8.0 * 1.2
        val my = (y1 - y0) / 9.0 * 1.2
        val cx0 = max(0.0, x0 - mx)
        val cy0 = max(0.0, y0 - my)
        val cx1 = min(frame.width.toDouble(), x1 + mx)
        val cy1 = min(frame.height.toDouble(), y1 + my)
        val w = ((cx1 - cx0).toInt() / 16) * 16
        val h = ((cy1 - cy0).toInt() / 16) * 16
        if (w < 64 || h < 64) return null
        return doubleArrayOf(cx0, cy0, cx0 + w, cy0 + h)
    }

    private fun handleFrame(frame: Frame) {
        if (paused || manualMode) return
        debugSaveFrameIfRequested(frame)
        val yolo = yoloDetector ?: run {
            yoloMissStreak++
            if (yoloMissStreak == 1 || yoloMissStreak % 20 == 0) {
                setStatus("识别引擎初始化失败：请尝试重启应用（重装可修复）")
            }
            return
        }
        val now = System.currentTimeMillis()
        if (now - lastRecognizeTs < MIN_RECOGNIZE_INTERVAL_MS) return
        lastRecognizeTs = now
        val crop = cropHint(frame)
        val mapped = try {
            yolo.detect(frame, crop)
        } catch (e: Exception) {
            Log.w(TAG, "yolo detect failed", e)
            null
        }
        if (mapped == null || mapped.pieceCount < MIN_RECOGNIZED_PIECES) {
            yoloMissStreak++
            sessionGrid = null
            if (yoloMissStreak == 1 || yoloMissStreak % 10 == 0) {
                setStatus("正在识别棋盘…\n请让棋盘完整无遮挡")
            }
            return
        }
        yoloMissStreak = 0
        sessionGrid = mapped.grid
        val result = RecognitionResult(
            mapped.canonical, mapped.screenRaw, mapped.orientation,
            AssistBoard.validate(mapped.canonical), 0, mapped.pieceCount, mapped.avgScore)
        Log.d(TAG, "rec(yolo): pieces=${mapped.pieceCount} orient=${mapped.orientation} " +
                "issue=${result.issues.size} dropped=${mapped.dropped}")
        when (tracker.onFrame(result)) {
            BoardTracker.Event.NEW_BOARD, BoardTracker.Event.NEW_GAME -> {
                cropUnstableStreak = 0
                onBoardConfirmed()
            }
            BoardTracker.Event.UNSTABLE -> {
                if (crop != null) {
                    cropUnstableStreak++
                    if (cropUnstableStreak >= 10) {
                        cropUnstableStreak = 0
                        sessionGrid = null
                    }
                } else {
                    cropUnstableStreak = 0
                }
                if (tracker.unstableStreak >= 15) {
                    setStatus("识别不稳定\n请让棋盘完整无遮挡\n画面稳定后自动恢复")
                }
            }
            BoardTracker.Event.SAME_BOARD -> { }
        }
    }

    private fun onBoardConfirmed() {
        val confirmed = tracker.confirmed ?: return
        currentCanonical = confirmed.canonical
        currentPieces = flatten(confirmed.canonical)
        currentOrientation = confirmed.orientation
        currentFen = AssistBoard.toFen(confirmed.canonical, tracker.redGo)
        resetSuggestionState()
        history.addLast(HistoryEntry(confirmed, tracker.redGo, currentFen))
        if (history.size > 60) history.removeFirst()
        Log.i(TAG, "board confirmed: $currentFen")
        postRender()
        scheduleAnalysis()
    }

    private fun flatten(p: Array<IntArray>): IntArray {
        val out = IntArray(90)
        for (y in 0 until 10) for (x in 0 until 9) out[y * 9 + x] = p[y][x]
        return out
    }

    private fun resetSuggestionState() {
        lastAnalysis = null
        selectedCandidate = 0
        bookMoves = null
        stableUcci = null
        stableSince = 0L
        searchSettled = false
    }

    private fun describeSafe(fen: String, ucci: String): String =
        try { MoveChinese.describe(fen, ucci) } catch (e: Exception) { ucci }

    private fun scheduleAnalysis() {
        workerHandler.post {
            val canonical = currentCanonical ?: return@post
            val fen = currentFen
            if (fen.isEmpty()) return@post
            val redGo = tracker.redGo
            val myTurn = redGo == mySideIsRed()
            if (myTurn) {
                val moves = queryBook(canonical, redGo)
                if (!moves.isNullOrEmpty()) {
                    bookMoves = moves
                    mainHandler.post { postRender() }
                    return@post
                }
            }
            bookMoves = null
            if (!engineReady) {
                mainHandler.post { setStatus(engineErrorText ?: "引擎预热中…") }
                return@post
            }
            analysisEngine?.request(fen)
            if (myTurn) mainHandler.post { setStatus("引擎思考中…") }
        }
    }

    private fun queryBook(canonical: Array<IntArray>, redGo: Boolean): List<BookData>? {
        if (openBookFailed) return null
        val book = openBook ?: try {
            BHOpenBook(this).also { openBook = it }
        } catch (t: Throwable) {
            Log.w(TAG, "openbook init failed", t)
            openBookFailed = true
            return null
        }
        return try {
            book.query(Zobrist.getZobristFromBoard(canonical, redGo), redGo, OpenBook.SortRule.BEST_SCORE)
        } catch (t: Throwable) {
            Log.w(TAG, "book query failed", t)
            null
        }
    }

    private val analysisListener = object : AnalysisEngine.Listener {
        override fun onEngineReady() {
            engineReady = true
            engineErrorText = null
            setStatus("识别中")
            scheduleAnalysis()
        }

        override fun onSearchUpdate(result: AnalysisResult) {
            if (result.fen != currentFen) return
            lastAnalysis = result
            val best = result.lines.firstOrNull()?.pv?.firstOrNull() ?: result.bestUcci
            if (best != stableUcci) {
                stableUcci = best
                stableSince = System.currentTimeMillis()
                searchSettled = false
            }
            postRender()
        }

        override fun onSearchDone(result: AnalysisResult) {
            if (result.fen != currentFen) return
            lastAnalysis = result
            searchSettled = true
            mainHandler.post {
                setStatus("识别中")
                postRender()
            }
        }

        override fun onEngineError(message: String) {
            Log.w(TAG, "engine error: $message")
            if (overlayClosed) return
            engineReady = false
            val short = if (message.length > 40) message.take(40) + "…" else message
            engineErrorText = short
            setStatus("引擎异常：$short")
        }
    }

    // ==================== 悬浮窗 ====================

    private fun ensureOverlay(): OverlayPanelView? {
        if (overlayPanel != null) return overlayPanel
        if (!Settings.canDrawOverlays(this)) return null
        val wm = getSystemService(WINDOW_SERVICE) as WindowManager
        val panel = OverlayPanelView(this)
        val dm = screenMetrics()
        val (px0, py0) = config.overlayPos
        val panelW = panel.dp(396)
        val panelH = panel.dp(192)
        val x = (if (px0 < 0) (dm.widthPixels - panelW) / 2 else (px0 * dm.widthPixels).toInt())
            .coerceIn(0, max(0, dm.widthPixels - panelW))
        val y = (if (py0 < 0) dm.heightPixels - panelH else (py0 * dm.heightPixels).toInt())
            .coerceIn(0, max(0, dm.heightPixels - panelH))
        val params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            this.x = x
            this.y = y
        }
        panel.onAction = ::onOverlayAction
        panel.onCellTap = ::onManualCellTap
        panel.onDragStateChange = { dragging -> userDragging = dragging }
        panel.setDragHandle(params, wm) {
            config.overlayPos = params.x.toFloat() / dm.widthPixels to params.y.toFloat() / dm.heightPixels
        }
        wm.addView(panel, params)
        overlayPanel = panel
        overlayParams = params
        windowManager = wm
        return panel
    }

    private fun overlayIntersectsBoard(): Boolean {
        val panel = overlayPanel ?: return false
        val grid = sessionGrid ?: return false
        val params = overlayParams ?: return false
        if (panel.width <= 0 || panel.height <= 0) return false
        val (fw, fh) = config.frameSize
        if (fw <= 0 || fh <= 0) return false
        val bx0 = grid.nx0 * fw
        val by0 = grid.ny0 * fh
        val bx1 = grid.nx1 * fw
        val by1 = grid.ny1 * fh
        val px0 = params.x
        val py0 = params.y
        val px1 = px0 + panel.width
        val py1 = py0 + panel.height
        return px0 < bx1 && px1 > bx0 && py0 < by1 && py1 > by0
    }

    private fun onOverlayAction(action: OverlayAction) {
        when (action) {
            OverlayAction.PAUSE -> {
                paused = !paused
                setStatus(if (paused) "已停止指导\n点『继续』恢复" else "继续指导")
            }
            OverlayAction.CLOSE -> {
                overlayClosed = true
                overlayPanel?.let { p -> runCatching { windowManager?.removeView(p) } }
                overlayPanel = null
                paused = true
                releaseCapture()
                setStatus("已关闭（识别与录屏均已停止，可在连线页重新启动）")
                stopSelf()
            }
            OverlayAction.CYCLE_CANDIDATE -> {
                val n = maxOf(bookMoves?.size ?: 0, lastAnalysis?.lines?.size ?: 0)
                if (n > 0) {
                    selectedCandidate = (selectedCandidate + 1) % n
                    postRender()
                }
            }
            OverlayAction.UNDO -> undoOneStep()
            OverlayAction.MANUAL -> toggleManualMode()
            OverlayAction.FLIP_TURN -> {
                tracker.redGo = !tracker.redGo
                resetSuggestionState()
                val canonical = currentCanonical
                if (canonical != null) {
                    currentFen = AssistBoard.toFen(canonical, tracker.redGo)
                }
                postRender()
                scheduleAnalysis()
                setStatus("棋局已更新\n现${if (tracker.redGo == mySideIsRed()) "轮到我方走" else "轮到对方走"}")
            }
            OverlayAction.CYCLE_STRENGTH -> cycleStrength()
            OverlayAction.CYCLE_HASH -> cycleHash()
        }
    }

    private fun undoOneStep() {
        if (history.size <= 1) {
            setStatus("没有可悔的局面（会随对弈自动积累）")
            return
        }
        history.removeLast()
        val prev = history.last()
        tracker.restore(prev.result, prev.redGo)
        currentCanonical = prev.result.canonical
        currentPieces = flatten(prev.result.canonical)
        currentOrientation = prev.result.orientation
        currentFen = prev.fen
        resetSuggestionState()
        if (!manualMode) paused = true
        postRender()
        scheduleAnalysis()
        setStatus("已悔棋一步\n点『继续』恢复")
    }

    private fun toggleManualMode() {
        manualMode = !manualMode
        manualSelected = null
        if (manualMode) {
            if (currentCanonical == null) {
                val start = AssistBoard.canonicalStart()
                currentCanonical = start
                currentPieces = flatten(start)
                currentOrientation = Orientation.STANDARD
                currentFen = AssistBoard.toFen(start, tracker.redGo)
            }
            val canonical = currentCanonical
            if (canonical != null && history.lastOrNull()?.fen != currentFen) {
                pushManualHistory(canonical, tracker.redGo)
            }
            setStatus("手动模式（再点手动退出）\n点子选中→点目标格=走子\n再点选中子=删除\n『更新棋局』纠正轮次")
        } else {
            setStatus("已退出手动模式\n恢复自动识别")
        }
        postRender()
    }

    private fun onManualCellTap(x: Int, y: Int) {
        if (!manualMode) return
        val board = currentCanonical ?: return
        val sel = manualSelected
        if (sel == null) {
            if (board[y][x] != Piece.EMPTY) {
                manualSelected = x to y
                postRender()
            }
            return
        }
        if (sel == x to y) {
            val p = board[y][x]
            manualSelected = null
            if (p == Piece.WSHUAI || p == Piece.BJIANG) {
                setStatus("手动模式\n帅/将不可删除")
                postRender()
                return
            }
            board[y][x] = Piece.EMPTY
            applyManualBoard(board, flipTurn = false)
            return
        }
        val moving = board[sel.second][sel.first]
        manualSelected = null
        if (moving == Piece.EMPTY) {
            postRender()
            return
        }
        board[sel.second][sel.first] = Piece.EMPTY
        board[y][x] = moving
        applyManualBoard(board, flipTurn = true)
    }

    private fun pushManualHistory(board: Array<IntArray>, redGo: Boolean) {
        val result = RecognitionResult(
            AssistBoard.clone(board), AssistBoard.clone(board), currentOrientation,
            emptyList(), 0, countPieces(board), 1.0)
        history.addLast(HistoryEntry(result, redGo, AssistBoard.toFen(board, redGo)))
        if (history.size > 60) history.removeFirst()
    }

    private fun countPieces(board: Array<IntArray>): Int =
        board.sumOf { row -> row.count { it != Piece.EMPTY } }

    private fun applyManualBoard(board: Array<IntArray>, flipTurn: Boolean) {
        if (flipTurn) tracker.redGo = !tracker.redGo
        currentCanonical = board
        currentPieces = flatten(board)
        currentFen = AssistBoard.toFen(board, tracker.redGo)
        pushManualHistory(board, tracker.redGo)
        resetSuggestionState()
        postRender()
        scheduleAnalysis()
    }

    private fun cycleStrength() {
        strengthIndex = (strengthIndex + 1) % strengthLevels.size
        val (name, depth) = strengthLevels[strengthIndex]
        config.searchDepth = depth
        analysisEngine?.setSearchDepth(depth)
        postRender()
        setStatus("引擎强度：$name")
    }

    private fun strengthButtonText(): String =
        "深度" + strengthLevels[strengthIndex].second

    private fun cycleHash() {
        hashIndex = (hashIndex + 1) % hashLevels.size
        val mb = hashLevels[hashIndex]
        config.hashMb = mb
        analysisEngine?.setHash(mb)
        postRender()
        setStatus("引擎 Hash：${mb}MB")
    }

    private fun hashButtonText(): String =
        "Hash" + hashLevels[hashIndex]

    private fun mySideIsRed(): Boolean = currentOrientation == Orientation.STANDARD

    private fun setStatus(text: String) {
        lastStatusText = text
        mainHandler.post {
            renderOverlay(statusOverride = text)
            runCatching {
                val nm = getSystemService(NotificationManager::class.java)
                nm.notify(NOTIF_ID, buildNotification(text))
            }
        }
    }

    @Volatile private var lastStatusText = "未启动"

    private fun postRender() {
        mainHandler.post { renderOverlay() }
    }

    private fun renderOverlay(statusOverride: String? = null) {
        if (overlayClosed) return
        val panel = ensureOverlay() ?: return
        val fen = currentFen
        val myRed = mySideIsRed()
        val a = lastAnalysis

        val status = statusOverride ?: when {
            manualMode -> "手动模式"
            paused -> "已停止指导\n仍观察局面"
            engineErrorText != null -> "引擎异常：$engineErrorText"
            engineReady -> "识别中"
            else -> "引擎预热中…"
        }
        val turnRed = tracker.redGo
        val myTurn = if (fen.isEmpty()) false else (turnRed == myRed)

        val lines = a?.lines ?: emptyList()
        val line = if (lines.isNotEmpty()) lines[selectedCandidate.coerceIn(0, lines.size - 1)] else null

        var headText = when {
            fen.isEmpty() -> ""
            else -> "我方${if (myRed) "红方" else "黑方"}\n${if (myTurn) "轮到我方走" else "轮到对方走"}"
        }
        var arrowFrom: Pair<Int, Int>? = null
        var arrowTo: Pair<Int, Int>? = null
        var suggestMature = false
        val recHealthy = manualMode || (yoloMissStreak == 0 && tracker.unstableStreak < 5)
        val book = if (myTurn) bookMoves else null

        fun fromIsOurs(ucci: String): Boolean {
            val arr = currentPieces ?: return false
            if (ucci.length < 4) return false
            val fx = ucci[0] - 'a'
            val fy = 9 - (ucci[1] - '0')
            if (fx !in 0..8 || fy !in 0..9) return false
            val p = arr[fy * 9 + fx]
            return p in 1..14 && Piece.isRed(p) == myRed
        }

        when {
            myTurn && recHealthy && book != null && book.isNotEmpty() -> {
                val idx = selectedCandidate.coerceIn(0, book.size - 1)
                val bd = book[idx]
                val ucci = bd.move
                if (fromIsOurs(ucci)) {
                    suggestMature = true
                    val arrow = ucciToCells(ucci)
                    arrowFrom = arrow.first
                    arrowTo = arrow.second
                    val chs = describeSafe(fen, ucci)
                    val tag = if (idx > 0) "开局库备选${idx + 1} $chs ✓" else "开局库 $chs ✓"
                    val total = bd.winRate + bd.drawNum + bd.loseNum
                    val wrTxt = if (total > 0) "胜率约 ${(bd.winRate * 100 / total).toInt()}%" else "库内推荐"
                    headText = "$tag\n$wrTxt\n$headText"
                } else {
                    headText = "轮次可能识别有误\n走一步后自动纠正\n$headText"
                }
            }
            myTurn && recHealthy && line != null -> {
                val ucci = line.pv.firstOrNull() ?: a?.bestUcci
                if (ucci != null && fromIsOurs(ucci)) {
                    suggestMature = searchSettled ||
                            System.currentTimeMillis() - stableSince >= SUGGEST_STABLE_MS
                    val chs = describeSafe(fen, ucci)
                    val arrow = ucciToCells(ucci)
                    arrowFrom = arrow.first
                    arrowTo = arrow.second
                    val tag = if (selectedCandidate > 0) "备选${selectedCandidate + 1} $chs" else "建议 $chs"
                    val suffix = if (suggestMature) " ✓" else "（暂定）"
                    headText = "$tag$suffix\n${lineScoreText(line)} 深度${line.depth}\n$headText"
                } else if (ucci != null) {
                    headText = "轮次可能识别有误\n走一步后自动纠正\n$headText"
                }
            }
            !myTurn && line != null && line.pv.size >= 2 -> {
                val oppU = line.pv[0]
                val myU = line.pv[1]
                if (fromIsOurs(myU)) {
                    val oppDesc = describeSafe(fen, oppU)
                    val myDesc = describeSafe(AssistBoard.applyUcci(fen, oppU), myU)
                    headText = "若对方走$oppDesc\n我方应$myDesc\n$headText"
                }
            }
            myTurn && engineReady && fen.isNotEmpty() -> {
                headText = "引擎思考中…\n$headText"
            }
        }

        panel.render(OverlayModel(
            statusText = if (headText.isEmpty()) status else "$status\n$headText",
            pieces = currentPieces,
            arrowFrom = arrowFrom,
            arrowTo = arrowTo,
            flipped = currentOrientation == Orientation.FLIPPED,
            mySideIsRed = myRed,
            mature = suggestMature,
            paused = paused,
            strengthText = strengthButtonText(),
            hashText = hashButtonText(),
            manualMode = manualMode,
            selectedCell = manualSelected,
        ))
    }

    private fun lineScoreText(line: AnalysisLine): String {
        val adv = line.scoreText()
        return if (line.redScoreCp >= 0) "红方优势 $adv" else "红方劣势 $adv"
    }

    private fun ucciToCells(ucci: String): Pair<Pair<Int, Int>, Pair<Int, Int>> {
        if (ucci.length < 4) return Pair(0 to 0, 0 to 0)
        val fx = ucci[0] - 'a'
        val fy = 9 - (ucci[1] - '0')
        val tx = ucci[2] - 'a'
        val ty = 9 - (ucci[3] - '0')
        return Pair(fx to fy, tx to ty)
    }

    override fun onDestroy() {
        Log.i(TAG, "service destroyed")
        releaseCapture()
        yoloDetector?.close()
        yoloDetector = null
        analysisEngine?.shutdown()
        analysisEngine = null
        overlayPanel?.let { p ->
            windowManager?.removeView(p)
        }
        overlayPanel = null
        frameThread.quitSafely()
        workerThread.quitSafely()
        super.onDestroy()
    }

    private fun releaseCapture() {
        try { virtualDisplay?.release() } catch (e: Exception) { }
        virtualDisplay = null
        try { imageReader?.close() } catch (e: Exception) { }
        imageReader = null
        try { mediaProjection?.stop() } catch (e: Exception) { }
        mediaProjection = null
        stopForeground(STOP_FOREGROUND_REMOVE)
    }

    private fun OverlayPanelView.dp(v: Int): Int =
        (v * resources.displayMetrics.density).toInt()
}