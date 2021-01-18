package com.zengge.jadx.utils;

import jadx.core.utils.exceptions.JadxRuntimeException;
import org.jf.smali.Smali;
import org.jf.smali.SmaliOptions;
import org.slf4j.LoggerFactory;

public class SmaliUtils {
    static {
        LoggerFactory.getLogger(SmaliUtils.class);
    }

    public static void assembleDex(String str, String str2) {
        try {
            SmaliOptions smaliOptions = new SmaliOptions();
            smaliOptions.outputDexFile = str;
            Smali.assemble(smaliOptions, str2);
        } catch (Exception e) {
            throw new JadxRuntimeException("Smali assemble error", e);
        }
    }
}