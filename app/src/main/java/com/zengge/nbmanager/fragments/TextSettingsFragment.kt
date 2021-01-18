package com.zengge.nbmanager.fragments

import android.content.SharedPreferences
import android.os.Bundle
import com.github.koston.preference.ColorPreferenceFragmentCompat
import com.zengge.nbmanager.R

class TextSettingsFragment : ColorPreferenceFragmentCompat(), SharedPreferences.OnSharedPreferenceChangeListener {
    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.text_settings, rootKey)
    }

    override fun onSharedPreferenceChanged(sharedPreferences: SharedPreferences?, key: String?) {
    }
}