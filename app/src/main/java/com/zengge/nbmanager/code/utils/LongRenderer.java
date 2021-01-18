package com.zengge.nbmanager.code.utils;

import java.io.IOException;

public class LongRenderer {
    public static void writeSignedIntOrLongTo(IndentingWriter indentingWriter, long j) throws IOException {
        if (j < 0) {
            indentingWriter.write("-0x");
            indentingWriter.printLongAsHex(-j);
            if (j < -2147483648L) {
                indentingWriter.write(76);
                return;
            }
            return;
        }
        indentingWriter.write("0x");
        indentingWriter.printLongAsHex(j);
        if (j > 2147483647L) {
            indentingWriter.write(76);
        }
    }
}