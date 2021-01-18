package com.zengge.nbmanager.utils;

import org.jf.baksmali.Baksmali;
import org.jf.baksmali.BaksmaliOptions;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.dexbacked.DexBackedDexFile;

import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;

public class DoBakSmaliUtils {
    public static boolean doBaksmali(String inpuFile, String outputDir) {
        ArrayList arrayList = new ArrayList();
        arrayList.add(".");
        try {
            File file = new File(inpuFile);
            if (!file.exists()) {
                PrintStream printStream = System.err;
                printStream.println("Can't find the file " + inpuFile);
                return false;
            }
            DexBackedDexFile loadDexFile = DexFileFactory.loadDexFile(file, Opcodes.getDefault());
            int size = arrayList.size();
            String[] strArr = new String[size];
            for (int i = 0; i < size; i++) {
                strArr[i] = (String) arrayList.get(i);
            }
            Baksmali.disassembleDexFile(loadDexFile, new File(outputDir), Runtime.getRuntime().availableProcessors(), new BaksmaliOptions());
            return true;
        } catch (RuntimeException e) {
            System.err.println("\n\nUNEXPECTED TOP-LEVEL EXCEPTION:");
            e.printStackTrace();
            return false;
        } catch (Throwable th) {
            System.err.println("\n\nUNEXPECTED TOP-LEVEL ERROR:");
            th.printStackTrace();
            return false;
        }
    }
}