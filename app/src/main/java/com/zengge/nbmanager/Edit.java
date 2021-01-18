package com.zengge.nbmanager;

import java.io.IOException;
import java.io.OutputStream;
import java.util.List;

public interface Edit {
    public void read(List<String> data, byte[] input) throws IOException;

    public void write(String data, OutputStream output) throws IOException;
}
