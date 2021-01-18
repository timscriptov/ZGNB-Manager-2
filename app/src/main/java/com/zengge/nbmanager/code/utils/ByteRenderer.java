package com.zengge.nbmanager.code.utils;

import com.zengge.nbmanager.R;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;

public class ByteRenderer {
    public static void writeUnsignedTo(@NotNull IndentingWriter indentingWriter, byte b) throws IOException {
        indentingWriter.write("0x");
        indentingWriter.printLongAsHex((b & 255));
        indentingWriter.write(R.styleable.AppCompatTheme_windowFixedWidthMajor);
    }
}