package com.zengge.jadx.utils.files;

import com.android.dex.Dex;

import org.jetbrains.annotations.NotNull;

import java.nio.file.Path;

public class DexFile {
    private final Dex dexBuf;
    private final InputFile inputFile;
    private final String name;

    public DexFile(InputFile inputFile2, String str, Dex dex, Path path) {
        this.inputFile = inputFile2;
        this.name = str;
        this.dexBuf = dex;
    }

    public String getName() {
        return this.name;
    }

    public Dex getDexBuf() {
        return this.dexBuf;
    }

    public InputFile getInputFile() {
        return this.inputFile;
    }

    public @NotNull String toString() {
        String str;
        StringBuilder sb = new StringBuilder();
        sb.append(this.inputFile);
        if (this.name.isEmpty()) {
            str = "";
        } else {
            str = ':' + this.name;
        }
        sb.append(str);
        return sb.toString();
    }
}