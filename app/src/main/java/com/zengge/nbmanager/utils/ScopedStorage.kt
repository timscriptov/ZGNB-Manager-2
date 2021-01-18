package com.zengge.nbmanager.utils

import android.os.Environment
import java.io.File

object ScopedStorage {
    @JvmStatic
    val storageDirectory: File
        get() = Environment.getExternalStorageDirectory()
}