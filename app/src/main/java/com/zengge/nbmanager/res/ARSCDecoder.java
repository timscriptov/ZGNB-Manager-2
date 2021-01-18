package com.zengge.nbmanager.res;

import com.zengge.nbmanager.utils.LEDataInputStream;
import com.zengge.nbmanager.utils.LEDataOutputStream;

import org.jetbrains.annotations.NotNull;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

public class ARSCDecoder {
    public static final int ARSC_CHUNK_TYPE = 0x000c0002;
    public static final int CHECK_PACKAGE = 512;
    private final LEDataInputStream mIn;
    public StringBlock mTableStrings;
    int packageCount;
    byte[] buf;
    String name;
    int id;
    ByteArrayOutputStream byteOut = new ByteArrayOutputStream();

    private ARSCDecoder(InputStream arscStream) {
        this.mIn = new LEDataInputStream(arscStream);
    }

    public static @NotNull ARSCDecoder read(InputStream in) throws IOException {
        ARSCDecoder arsc = new ARSCDecoder(in);
        arsc.readTable();
        return arsc;
    }

    private void readTable() throws IOException {
        int type = mIn.readInt();
        checkChunk(type, ARSC_CHUNK_TYPE);
        mIn.readInt();// chunk size
        packageCount = this.mIn.readInt();
        this.mTableStrings = StringBlock.read(this.mIn);
        readPackage();
    }

    public void write(List<String> list, OutputStream out) throws IOException {
        write(list, new LEDataOutputStream(out));
    }

    public void write(List<String> list, @NotNull LEDataOutputStream out)
            throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        LEDataOutputStream buf = new LEDataOutputStream(baos);
        buf.writeInt(packageCount);
        mTableStrings.write(list, buf);
        writePackage(buf);
        // write to out
        out.writeInt(ARSC_CHUNK_TYPE);
        out.writeInt(baos.size() + 8);
        out.writeFully(baos.toByteArray());
    }

    public void writePackage(@NotNull LEDataOutputStream out) throws IOException {
        out.writeFully(byteOut.toByteArray());
    }

    private void readPackage() throws IOException {
        byte[] buf = new byte[2048];
        int num;
        while ((num = mIn.read(buf, 0, 2048)) != -1)
            byteOut.write(buf, 0, num);
    }

    private void checkChunk(int type, int expectedType) throws IOException {
        if (type != expectedType)
            throw new IOException(String.format(
                    "Invalid chunk type: expected=0x%08x, got=0x%08x",
                    new Object[]{Integer.valueOf(expectedType),
                            Short.valueOf((short) type)
                    }));
    }
}