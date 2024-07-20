package com.example.udptouchpad

import android.content.Context
import android.content.pm.ActivityInfo
import android.graphics.Color
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.StrictMode
import android.view.MotionEvent
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import java.lang.Integer.min
import java.net.DatagramPacket
import java.net.InetAddress
import java.net.MulticastSocket

class MainActivity : AppCompatActivity(), SensorEventListener {
    // storage for output data
    private val messageID: Byte = 0x42.toByte()
    private var counter: Byte = 0.toByte()
    @OptIn(ExperimentalUnsignedTypes::class)
    private var screenSize: UIntArray = UIntArray(2){0u}
    private var touchID: ByteArray = ByteArray(10){255.toByte()}
    private var touchX: FloatArray = FloatArray(10){0.0f}
    private var touchY: FloatArray = FloatArray(10){0.0f}
    private var rotationVector: FloatArray = FloatArray(3){Float.NaN}
    private var acceleration: FloatArray = FloatArray(3){Float.NaN}
    private var angularRate: FloatArray = FloatArray(3){Float.NaN}

    private lateinit var sensorManager: SensorManager
    private lateinit var mainHandler: Handler
    private lateinit var udpSocket: MulticastSocket
    private val destinationIP: String = "239.192.82.74"
    private var destinationAddress: InetAddress = InetAddress.getByName(destinationIP)
    private var destinationPort: Int = 10891
    private var multicastTTL: Int = 1
    private var txBuffer: ByteArray = ByteArray(256) // at least 136
    private var networkOK: Boolean = false
    private lateinit var textViewNetworkStatus: TextView
    private lateinit var textViewDebugInfo: TextView
    private lateinit var textDestination: TextView

    @OptIn(ExperimentalUnsignedTypes::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            val windowInsetsController = WindowCompat.getInsetsController(window, window.decorView)
            windowInsetsController.hide(WindowInsetsCompat.Type.systemBars())
            insets
        }
        textViewNetworkStatus = findViewById<TextView>(R.id.text_network_status)
        textViewNetworkStatus.text = "NETWORK OFF"
        textViewNetworkStatus.setTextColor(Color.rgb(255,0,0))
        textViewDebugInfo = findViewById<TextView>(R.id.text_debug)
        textViewDebugInfo.text = ""
        textViewDebugInfo.setTextColor(Color.rgb(255,128,0))
        textDestination = findViewById<TextView>(R.id.text_destination)
        textDestination.text = destinationIP + ":" + destinationPort.toString()
        this.requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        sensorManager = getSystemService(Context.SENSOR_SERVICE) as SensorManager
        val sensorRotationVector = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)
        val sensorAccelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
        val sensorGyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE)
        if(sensorRotationVector != null){
            sensorManager.registerListener(this, sensorRotationVector, SensorManager.SENSOR_DELAY_GAME)
        }
        if(sensorAccelerometer != null){
            sensorManager.registerListener(this, sensorAccelerometer, SensorManager.SENSOR_DELAY_GAME) // faster than SENSOR_DELAY_GAME does not work
        }
        if(sensorGyroscope != null){
            sensorManager.registerListener(this, sensorGyroscope, SensorManager.SENSOR_DELAY_GAME) // faster than SENSOR_DELAY_GAME does not work
        }
        this.screenSize[0] = resources.displayMetrics.widthPixels.toUInt()
        this.screenSize[1] = resources.displayMetrics.heightPixels.toUInt()
        mainHandler = Handler(Looper.getMainLooper())

        StrictMode.setThreadPolicy(StrictMode.ThreadPolicy.Builder().permitAll().build())
        try {
            udpSocket = MulticastSocket()
            udpSocket.timeToLive = multicastTTL
            udpSocket.broadcast = true
        }
        catch (ex: Exception){
            textViewDebugInfo.text = ex.toString()
        }
    }

    override fun onPause() {
        super.onPause()
        mainHandler.removeCallbacks(updateTask)
    }

    override fun onResume() {
        super.onResume()
        mainHandler.post(updateTask)
    }

    override fun onSensorChanged(event: SensorEvent?) {
        if(event != null){
            if(Sensor.TYPE_ROTATION_VECTOR == event.sensor.type){
                this.rotationVector = event.values
            }
            else if(Sensor.TYPE_ACCELEROMETER == event.sensor.type){
                this.acceleration = event.values
            }
            else if(Sensor.TYPE_GYROSCOPE == event.sensor.type){
                this.angularRate = event.values
            }
        }
    }

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if(event != null) {
            val actionMasked = event.actionMasked
            touchX.fill(0.0f)
            touchY.fill(0.0f)
            touchID.fill(255.toByte())
            for (i in 0 until min(event.pointerCount, 10)) {
                touchX[i] = event.getX(i)
                touchY[i] = event.getY(i)
                touchID[i] = event.getPointerId(i).toByte()
            }
            if(MotionEvent.ACTION_UP == actionMasked || MotionEvent.ACTION_POINTER_UP == actionMasked){
                val i = event.actionIndex
                touchX[i] = 0.0f
                touchY[i] = 0.0f
                touchID[i] = 255.toByte()
            }
            this.update()
        }
        return super.onTouchEvent(event)
    }

    private val updateTask = object : Runnable {
        override fun run() {
            update()
            mainHandler.postDelayed(this, 20)
        }
    }

    @OptIn(ExperimentalUnsignedTypes::class)
    private fun update() {
        try {
            // pack a new message
            txBuffer[0] = messageID
            txBuffer[1] = counter++
            if(txBuffer[1].toInt() == 0xff) { // required because increment does not go to zero and operator++ does not work for null-safety (Byte?) types
                counter = 0
            }
            var numBytes: Int = 2
            for(k in 0..1) {
                for (i in 0..3) txBuffer[numBytes + i] = (screenSize[k] shr ((3-i) * 8)).toByte()
                numBytes += 4
            }
            for(k in 0..9) {
                txBuffer[numBytes++] = touchID[k];
            }
            for(k in 0..9) {
                val bitsX: Int = touchX[k].toRawBits()
                for (i in 0..3) txBuffer[numBytes + i] = (bitsX shr ((3-i) * 8)).toByte()
                numBytes += 4
                val bitsY: Int = touchY[k].toRawBits()
                for (i in 0..3) txBuffer[numBytes + i] = (bitsY shr ((3-i) * 8)).toByte()
                numBytes += 4
            }
            for(k in 0..2) {
                val bits: Int = rotationVector[k].toRawBits()
                for (i in 0..3) txBuffer[numBytes + i] = (bits shr ((3-i) * 8)).toByte()
                numBytes += 4
            }
            for(k in 0..2) {
                val bits: Int = acceleration[k].toRawBits()
                for (i in 0..3) txBuffer[numBytes + i] = (bits shr ((3-i) * 8)).toByte()
                numBytes += 4
            }
            for(k in 0..2) {
                val bits: Int = angularRate[k].toRawBits()
                for (i in 0..3) txBuffer[numBytes + i] = (bits shr ((3-i) * 8)).toByte()
                numBytes += 4
            }

            // send message
            val packet = DatagramPacket(txBuffer, numBytes, destinationAddress, destinationPort)
            udpSocket.send(packet)
            if(!networkOK){
                textViewNetworkStatus.text = "NETWORK ON"
                textViewNetworkStatus.setTextColor(Color.rgb(0,255,0))
            }
            networkOK = true
        }
        catch (ex: Exception){
            if(networkOK){
                textViewNetworkStatus.text = "NETWORK OFF"
                textViewNetworkStatus.setTextColor(Color.rgb(255,0,0))
            }
            networkOK = false
        }
    }

    override fun onAccuracyChanged(p0: Sensor?, p1: Int) {}
}
