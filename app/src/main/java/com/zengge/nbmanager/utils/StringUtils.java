package com.zengge.nbmanager.utils;

import org.jetbrains.annotations.NotNull;

import java.util.Collection;
import java.util.Iterator;

public class StringUtils {

    public static @NotNull String join(@NotNull Collection<String> collection, String delimiter) {
        StringBuffer buffer = new StringBuffer();
        Iterator<String> iter = collection.iterator();
        while (iter.hasNext()) {
            buffer.append(iter.next());
            if (iter.hasNext())
                buffer.append(delimiter);
        }
        return buffer.toString();
    }
}
