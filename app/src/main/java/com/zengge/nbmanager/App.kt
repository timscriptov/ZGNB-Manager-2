package com.zengge.nbmanager

import android.annotation.SuppressLint
import android.app.Application
import android.content.Context
import android.content.SharedPreferences
import android.util.DisplayMetrics
import android.widget.Toast
import androidx.multidex.MultiDex
import androidx.preference.PreferenceManager
import org.jetbrains.annotations.Nullable
import kotlin.math.roundToInt

class App : Application() {
    override fun attachBaseContext(base: Context) {
        super.attachBaseContext(base)
        MultiDex.install(this)
    }

    override fun onCreate() {
        super.onCreate()
        context = applicationContext
        try {
            System.loadLibrary("function")
        } catch (u: UnsatisfiedLinkError) {
            u.printStackTrace()
            try {
                Thread.sleep(3000)
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }
            System.exit(-1)
        }
    }

    companion object {
        @SuppressLint("StaticFieldLeak")
        private var context: Context? = null
        private var app: Application? = null
        private var preferences: SharedPreferences? = null

        @JvmStatic
        fun getContext(): Context? {
            if (context == null) {
                context = App()
            }
            return context
        }

        fun getApp(): Application? {
            if (app == null) {
                app = App()
            }
            return app
        }

        @JvmStatic
        @Nullable
        fun getPreferences(): SharedPreferences {
            if (preferences == null) {
                preferences = PreferenceManager.getDefaultSharedPreferences(this.getContext())
            }
            return preferences!!
        }
    }
}