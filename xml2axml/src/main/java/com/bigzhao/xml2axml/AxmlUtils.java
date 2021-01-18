package com.bigzhao.xml2axml;

import com.bigzhao.xml2axml.func.AXMLPrinter;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;
import org.jetbrains.annotations.Nullable;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;

/**
 * Created by Roy on 16-4-27.
 */
public class AxmlUtils {

    public static @Nullable String decode(byte[] data) {
        try (InputStream is = new ByteArrayInputStream(data)) {
            ByteArrayOutputStream os = new ByteArrayOutputStream();
            AXMLPrinter.out = new PrintStream(os);
            AXMLPrinter.decode(is);
            byte[] bs = os.toByteArray();
            IOUtils.closeQuietly(os);
            AXMLPrinter.out.close();
            return new String(bs, StandardCharsets.UTF_8);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public static String decode(File file) throws IOException {
        return decode(FileUtils.readFileToByteArray(file));
    }

    public static byte @Nullable [] encode(String xml) {
        try {
            Encoder encoder = new Encoder();
            return encoder.encodeString(null, xml);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public static byte @Nullable [] encode(File file) {
        try {
            Encoder encoder = new Encoder();
            return encoder.encodeFile(null, file.getAbsolutePath());
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}
