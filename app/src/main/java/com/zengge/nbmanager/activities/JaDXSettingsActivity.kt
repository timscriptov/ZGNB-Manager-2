package com.zengge.nbmanager.activities

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.zengge.nbmanager.R
import com.zengge.nbmanager.fragments.JaDXSettingsFragment

class JaDXSettingsActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.settings)
        //setSupportActionBar(findViewById(R.id.toolbar))
        //supportActionBar!!.setDisplayHomeAsUpEnabled(true)
        supportFragmentManager
                .beginTransaction()
                .add(R.id.frame_container, JaDXSettingsFragment())
                .commit()
    }
}