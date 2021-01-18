package com.zengge.nbmanager.ui

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.view.View
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AlertDialog
import com.keenfin.audioview.AudioView
import com.zengge.nbmanager.BuildConfig
import com.zengge.nbmanager.R
import java.io.IOException

object Dialogs {
    var contentUrl = ""

    @RequiresApi(Build.VERSION_CODES.R)
    @JvmStatic
    fun showScopedStorageDialog(context: Context) {
        AlertDialog.Builder(context)
                .setTitle(R.string.scoped_storage_title)
                .setMessage(R.string.scoped_storage_msg)
                .setPositiveButton(R.string.settings_title) { p1, p2 ->
                    val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + BuildConfig.APPLICATION_ID))
                    context.startActivity(intent)
                }
                .create().show()
    }

    @JvmStatic
    fun showAudioPlayerDialog(context: Context, activity: Activity) {
        AlertDialog.Builder(context)
                .setTitle(R.string.scoped_storage_title)
                .setMessage(R.string.scoped_storage_msg)
                .setPositiveButton(R.string.settings_title) { p1, p2 ->
                    contentUrl = activity.getIntent().getStringExtra("AUDIOPATH").toString()
                    try {
                        (activity.findViewById<View>(R.id.live) as AudioView).setDataSource(contentUrl)
                    } catch (e: IOException) {
                        e.printStackTrace()
                    }
                }
                .create().show()
    }
}