package com.zengge.nbmanager.activities

import android.content.Intent
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.keenfin.audioview.AudioService
import com.keenfin.audioview.AudioView
import com.zengge.nbmanager.R
import java.io.IOException

class AudioPlayerActivity : AppCompatActivity() {
    var contentUrl: String? = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.audio_player)
        contentUrl = intent.getStringExtra("AUDIOPATH")
        try {
            (findViewById<View>(R.id.live) as AudioView).setDataSource(contentUrl)
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        val audioService = Intent(this, AudioService::class.java)
        audioService.action = AudioService.ACTION_STOP_AUDIO
        stopService(audioService)
    }
}