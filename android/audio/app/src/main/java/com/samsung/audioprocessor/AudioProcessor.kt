import kotlinx.coroutines.*
import kotlinx.coroutines.channels.Channel
import java.io.PipedInputStream
import java.io.PipedOutputStream
import java.util.concurrent.atomic.AtomicReference
import kotlin.coroutines.CoroutineContext
import android.content.Context
import android.util.Log

enum class Tag { START, CONTINUE, END }
enum class State { READY, PREPARING_STREAMING, STREAMING_REQUEST, STREAMING, PROCESSING }

data class AudioPacket(val buffer: ByteArray, val tag: Tag)

class AudioProcessor(
    private val androidContext: Context,
    private val coroutineContext: CoroutineContext = Dispatchers.Default
) {
    private val queue = Channel<AudioPacket>(Channel.UNLIMITED)
    private val scope = CoroutineScope(coroutineContext + SupervisorJob())
    private val state = AtomicReference(State.READY)

    private var outputStream: PipedOutputStream? = null
    private var inputStream: PipedInputStream? = null
    private var isProcessing = false

    init {
        scope.launch { processQueue() }
    }

    fun enqueue(buffer: ByteArray, tag: Tag) {
        scope.launch {
            try {
                queue.send(AudioPacket(buffer, tag))
            } catch (e: Exception) {
                Log.e("AudioProcessor", "Error enqueueing audio packet: ${e.message}")
            }
        }
    }

    private suspend fun processQueue() {
        try {
            while (scope.isActive) {
                val currentState = state.get()
                when (currentState) {
                    State.READY -> {
                        prepareStreaming()
                        transitionToState(State.STREAMING_REQUEST)
                    }
                    State.STREAMING -> {
                        handleStreamingState()
                    }
                    State.PROCESSING -> {
                        // Just transition to READY - cleanup is not needed
                        transitionToState(State.READY)
                    }
                    else -> {
                        // For other states, wait a bit before checking again
                        delay(10)
                    }
                }
            }
        } catch (e: Exception) {
            Log.e("AudioProcessor", "Error processing audio queue: ${e.message}")
        }
    }

    private suspend fun handleStreamingState() {
        // Process packets continuously until END packet or queue is empty
        while (scope.isActive && state.get() == State.STREAMING) {
            try {
                val packet = queue.receive()
                when (packet.tag) {
                    Tag.START -> processAudioData(packet.buffer)
                    Tag.CONTINUE -> processAudioData(packet.buffer)
                    Tag.END -> {
                        processAudioData(packet.buffer)
                        startProcessing()
                        transitionToState(State.PROCESSING)
                    }
                }
            } catch (e: kotlinx.coroutines.channels.ClosedReceiveChannelException) {
                // Queue is closed, break out of inner loop
                break
            }
        }
    }

    private suspend fun processAudioData(buffer: ByteArray) {
        try {
            outputStream?.write(buffer)
            outputStream?.flush()
        } catch (e: Exception) {
            Log.e("AudioProcessor", "Error processing audio data: ${e.message}")
        }
    }

    private fun prepareStreaming() {
        try {
            outputStream = PipedOutputStream()
            inputStream = PipedInputStream(outputStream)
            Log.d("AudioProcessor", "Streaming prepared successfully")
        } catch (e: Exception) {
            Log.e("AudioProcessor", "Error preparing streaming: ${e.message}")
        }
    }

    private fun startProcessing() {
        isProcessing = true
        try {
            outputStream?.close()
            Log.d("AudioProcessor", "Audio processing started - output stream closed")
        } catch (e: Exception) {
            Log.e("AudioProcessor", "Error closing output stream: ${e.message}")
        }
    }

    private fun transitionToState(newState: State) {
        val currentState = state.get()
        if (isValidTransition(currentState, newState)) {
            state.set(newState)
            Log.d("AudioProcessor", "State transition: $currentState -> $newState")
        } else {
            Log.w("AudioProcessor", "Invalid state transition: $currentState -> $newState")
        }
    }

    private fun isValidTransition(from: State, to: State): Boolean {
        return when (from) {
            State.READY -> to == State.STREAMING_REQUEST
            State.STREAMING_REQUEST -> to == State.STREAMING
            State.STREAMING -> to == State.PROCESSING
            State.PROCESSING -> to == State.READY
        }
    }


    fun getCurrentState(): State = state.get()

    fun getInputStream(): PipedInputStream? = inputStream

    fun isReady(): Boolean = state.get() == State.READY

    fun isProcessing(): Boolean = isProcessing && state.get() == State.PROCESSING

    fun getContext(): Context = androidContext

    fun stop() {
        scope.cancel()
        queue.close()
        transitionToState(State.READY)
    }
}

