package com.zengge.nbmanager.data

import androidx.preference.PreferenceManager
import com.zengge.nbmanager.App

object Preferences {
    private val preferences = PreferenceManager.getDefaultSharedPreferences(App.getContext())

    @JvmStatic
    var skipResources: Boolean
        get() = preferences.getBoolean("skip_resources", true)
        set(mode) {
            preferences.edit().putBoolean("skip_resources", mode).apply()
        }

    @JvmStatic
    var showInconsistentCode: Boolean
        get() = preferences.getBoolean("show_inconsistent_code", true)
        set(mode) {
            preferences.edit().putBoolean("show_inconsistent_code", mode).apply()
        }
}