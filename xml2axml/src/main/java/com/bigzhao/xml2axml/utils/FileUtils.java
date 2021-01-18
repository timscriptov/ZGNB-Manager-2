package com.bigzhao.xml2axml.utils;

import java.io.FileOutputStream;
import java.io.IOException;

public class FileUtils {
    public static void writeBytesToFile(String fileOutput, byte[] bytes)
            throws IOException {
        try (FileOutputStream fos = new FileOutputStream(fileOutput)) {
            fos.write(bytes);
        }
    }
}
