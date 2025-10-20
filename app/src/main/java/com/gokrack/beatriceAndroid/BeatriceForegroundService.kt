package com.gokrack.beatriceAndroid

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.util.Log
import com.gokrack.beatriceAndroid.R

class BeatriceForegroundService : Service() {

    companion object {
        private const val TAG = "BeatriceFS"
        const val ACTION_START = "ACTION_START"
        const val ACTION_STOP = "ACTION_STOP"
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    private fun buildNotification(): Notification? {
        val manager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                "all",
                "All Notifications",
                NotificationManager.IMPORTANCE_NONE
            )
            manager.createNotificationChannel(channel)

            Notification.Builder(this, "all")
                .setContentTitle("Playing/recording audio")
                .setContentText("playing/recording...")
                .setSmallIcon(R.mipmap.ic_launcher)
                .build()
        } else {
            null
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.i(TAG, "Receive onStartCommand $intent")

        when (intent?.action) {
            ACTION_START -> {
                Log.i(TAG, "Receive ACTION_START ${intent.extras}")
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    buildNotification()?.let {
                        startForeground(
                            1,
                            it,
                            ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK or
                                    ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE
                        )
                    }
                }
            }
            ACTION_STOP -> {
                Log.i(TAG, "Receive ACTION_STOP ${intent.extras}")
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    stopForeground(STOP_FOREGROUND_REMOVE)
                }
            }
        }

        return START_NOT_STICKY
    }
}