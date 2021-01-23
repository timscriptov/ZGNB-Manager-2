package com.zengge.nbmanager

object Features {
    @JvmStatic
    external fun ExtractAllRAR(f: String?, d: String?): Int
    @JvmStatic
    external fun Oat2Dex(f: String?): Boolean
    @JvmStatic
    external fun Odex2Dex(file: String?, dest: String?): Boolean
    @JvmStatic
    external fun ZipAlign(zip: String?, destZip: String?): Boolean
    @JvmStatic
    external fun isZipAligned(zip: String?): Boolean
    @JvmStatic
    external fun isValidElf(elf: String?): Boolean
    @JvmStatic
    external fun compressStrToInt(str: String?): String?
    @JvmStatic
    external fun ELFHash(strUri: String?): Long
    @JvmStatic
    external fun dumpDex(apiLevel: Int, appEntry: String?): Int
    @JvmStatic
    external fun AStyleMain(text: String?, opt: String?): String?
}