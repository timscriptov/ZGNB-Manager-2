package com.zengge.nbmanager.code.utils;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.io.Writer;

public class IndentingWriter extends Writer {
    private static final String newLine = System.getProperty("line.separator");
    private final char[] buffer = new char[16];
    private final StringBuilder writer;
    private boolean beginningOfLine;
    private int indentLevel = 0;

    public IndentingWriter(StringBuilder sb) {
        writer = sb;
    }

    public void close() throws IOException {
    }

    public void flush() throws IOException {
    }

    public void write(int i) throws IOException {
        if (i == 10) {
            writer.append(newLine);
            beginningOfLine = true;
            return;
        }
        if (this.beginningOfLine) {
            for (int i2 = 0; i2 < indentLevel; i2++) {
                writer.append(' ');
            }
        }
        beginningOfLine = false;
        writer.append((char) i);
    }

    public void write(char @NotNull [] cArr) throws IOException {
        for (char write : cArr) {
            write(write);
        }
    }

    public void write(char[] cArr, int i, int i2) throws IOException {
        int i3 = i2 + i;
        while (i < i3) {
            write(cArr[i]);
            i++;
        }
    }

    public void write(@NotNull String str) throws IOException {
        int length = str.length();
        for (int i = 0; i < length; i++) {
            write(str.charAt(i));
        }
    }

    public void write(String str, int i, int i2) throws IOException {
        int i3 = i2 + i;
        while (i < i3) {
            write(str.charAt(i));
            i++;
        }
    }

    public @NotNull Writer append(@NotNull CharSequence charSequence) throws IOException {
        write(charSequence.toString());
        return this;
    }

    public @NotNull Writer append(@NotNull CharSequence charSequence, int i, int i2) throws IOException {
        write(charSequence.subSequence(i, i2).toString());
        return this;
    }

    public @NotNull Writer append(char c) throws IOException {
        write((int) c);
        return this;
    }

    public void indent(int i) {
        int i2 = indentLevel + i;
        this.indentLevel = i2;
        if (i2 < 0) {
            indentLevel = 0;
        }
    }

    public void deindent(int i) {
        int i2 = indentLevel - i;
        this.indentLevel = i2;
        if (i2 < 0) {
            indentLevel = 0;
        }
    }

    public void printLongAsHex(long j) throws IOException {
        int i;
        int i2 = 0;
        do {
            int i3 = (int) (15 & j);
            if (i3 < 10) {
                i = i2 + 1;
                buffer[i2] = (char) (i3 + 48);
            } else {
                i = i2 + 1;
                buffer[i2] = (char) ((i3 - 10) + 97);
            }
            i2 = i;
            j >>>= 4;
        } while (j != 0);
        while (i2 > 0) {
            i2--;
            write(buffer[i2]);
        }
    }

    public void printIntAsDec(int i) throws IOException {
        int i2;
        int i3 = 0;
        boolean z = i < 0;
        while (true) {
            i2 = i3 + 1;
            buffer[i3] = (char) ((i % 10) + 48);
            i /= 10;
            if (i == 0) {
                break;
            }
            i3 = i2;
        }
        if (z) {
            write(45);
        }
        while (i2 > 0) {
            i2--;
            write(buffer[i2]);
        }
    }
}