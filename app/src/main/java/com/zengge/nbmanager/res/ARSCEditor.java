package com.zengge.nbmanager.res;

import com.zengge.nbmanager.Edit;

import org.jetbrains.annotations.NotNull;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public class ARSCEditor implements Edit {
    private ARSCDecoder arsc;

    public ARSCEditor() {
    }

    public void read(final List<String> data, byte[] input) throws IOException {
        arsc = ARSCDecoder.read(new ByteArrayInputStream(input));
        arsc.mTableStrings.getStrings(data);
    }

    public void write(@NotNull String data, OutputStream out) throws IOException {
        String[] strings = data.split("\n");
        List<String> list = new ArrayList<String>(strings.length);
        for (String str : strings)
            list.add(str);
        arsc.write(list, out);
    }
}
