package com.zengge.nbmanager.utils;

import org.jf.baksmali.Baksmali;
import org.jf.baksmali.BaksmaliOptions;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.iface.DexFile;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class DoBakSmaliUtils {
    public static String[] bootClassPathDirsArray;

    public static boolean doBaksmali(String infile, String outdir) {
        List<String> bootClassPathDirs = new ArrayList<>();
        bootClassPathDirs.add(".");
        try {
            File dexFileFile = new File(infile);
            if(!dexFileFile.exists()) {
                System.err.println("Can't find the file " + infile);
                return false;
            }
            //Read in and parse the dex file
            DexFile dexFile = DexFileFactory.loadDexFile(dexFileFile, Opcodes.getDefault());
            bootClassPathDirsArray = new String[bootClassPathDirs.size()];
            for(int i = 0; i < bootClassPathDirsArray.length; i++)
                bootClassPathDirsArray[i] = bootClassPathDirs.get(i);
            BaksmaliOptions bo = new BaksmaliOptions();
            Baksmali.disassembleDexFile(dexFile, new File(outdir), Runtime.getRuntime().availableProcessors(), bo);
        } catch(RuntimeException ex) {
            System.err.println("\n\nUNEXPECTED TOP-LEVEL EXCEPTION:");
            ex.printStackTrace();
            return false;
        } catch(Throwable ex) {
            System.err.println("\n\nUNEXPECTED TOP-LEVEL ERROR:");
            ex.printStackTrace();
            return false;
        }
        return true;
    }
}