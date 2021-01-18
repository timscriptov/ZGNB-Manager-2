package com.zengge.nbmanager.res;

import com.zengge.nbmanager.utils.LEDataInputStream;
import com.zengge.nbmanager.utils.LEDataOutputStream;

import org.jetbrains.annotations.NotNull;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

public class AXmlDecoder {
    private static final int AXML_CHUNK_TYPE = 0x00080003;
    private final LEDataInputStream mIn;
    public StringBlock mTableStrings;
    ByteArrayOutputStream byteOut = new ByteArrayOutputStream();

    private AXmlDecoder(LEDataInputStream in) {
        this.mIn = in;
    }

    public static @NotNull AXmlDecoder read(InputStream input) throws IOException {
        AXmlDecoder axml = new AXmlDecoder(new LEDataInputStream(input));
        axml.readStrings();
        return axml;
    }

    private void readStrings() throws IOException {
        int type = mIn.readInt();
        checkChunk(type, AXML_CHUNK_TYPE);
        mIn.readInt();// Chunk size
        mTableStrings = StringBlock.read(this.mIn);
        byte[] buf = new byte[2048];
        int num;
        while ((num = mIn.read(buf, 0, 2048)) != -1)
            byteOut.write(buf, 0, num);
    }

    public void write(List<String> list, OutputStream out) throws IOException {
        write(list, new LEDataOutputStream(out));
    }

    public void write(List<String> list, @NotNull LEDataOutputStream out)
            throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        LEDataOutputStream buf = new LEDataOutputStream(baos);
        mTableStrings.write(list, buf);
        buf.writeFully(byteOut.toByteArray());
        // write out
        out.writeInt(AXML_CHUNK_TYPE);
        out.writeInt(baos.size() + 8);
        out.writeFully(baos.toByteArray());
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
