package com.zengge.nbmanager.res;

import com.zengge.nbmanager.Edit;

import org.jetbrains.annotations.NotNull;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public class AXmlEditor implements Edit {
    private AXmlDecoder axml;

    public void read(final List<String> data, byte[] input) throws IOException {
        axml = AXmlDecoder.read(new ByteArrayInputStream(input));
        axml.mTableStrings.getStrings(data);
    }

    public void write(@NotNull String data, OutputStream out) throws IOException {
        String[] strings = data.split("\n");
        List<String> list = new ArrayList<String>(strings.length);
        for (String str : strings)
            list.add(str);
        axml.write(list, out);
    }
}
