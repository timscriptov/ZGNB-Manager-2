package com.zengge.jadx.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import org.objectweb.asm.ClassReader;

public class AsmUtils {
    public static String getNameFromClassFile(File file) throws IOException {
        FileInputStream fileInputStream = new FileInputStream(file);
        String className = new ClassReader(fileInputStream).getClassName();
        fileInputStream.close();
        return className;
    }
}