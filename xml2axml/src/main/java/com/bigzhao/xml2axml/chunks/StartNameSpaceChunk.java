package com.bigzhao.xml2axml.chunks;

import com.bigzhao.xml2axml.IntWriter;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;

/**
 * Created by Roy on 15-10-5.
 */
public class StartNameSpaceChunk extends Chunk<StartNameSpaceChunk.H> {

    public String prefix;
    public String uri;

    public StartNameSpaceChunk(Chunk parent) {
        super(parent);
    }

    @Override
    public void writeEx(@NotNull IntWriter w) throws IOException {
        w.write(stringIndex(null, prefix));
        w.write(stringIndex(null, uri));
    }

    public class H extends Chunk.NodeHeader {
        public H() {
            super(ChunkType.XmlStartNamespace);
            size = 0x18;
        }
    }
}
