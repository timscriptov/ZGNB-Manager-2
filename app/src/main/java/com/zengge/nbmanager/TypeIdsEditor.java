package com.zengge.nbmanager;

import com.zengge.nbmanager.activities.ClassListActivity;

import org.jetbrains.annotations.NotNull;
import org.jf.dexlib.TypeIdItem;
import org.jf.dexlib.Util.Utf8Utils;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public class TypeIdsEditor implements Edit {
    private ArrayList<TypeIdItem> typeIds;

    public void read(List<String> data, byte[] input) throws IOException {
        List<TypeIdItem> typeIds = ClassListActivity.dexFile.TypeIdsSection
                .getItems();
        for (TypeIdItem typeId : typeIds)
            data.add(typeId.getTypeDescriptor());
        this.typeIds = (ArrayList) typeIds;
    }

    public void write(@NotNull String data, OutputStream out) throws IOException {
        ArrayList<TypeIdItem> typeIds = this.typeIds;
        String[] strings = data.split("\n");
        for (int i = 0, len = typeIds.size(); i < len; i++) {
            TypeIdItem item = typeIds.get(i);
            item.setTypeDescriptor(Utf8Utils.escapeSequence(strings[i]));
        }
        ClassListActivity.isChanged = true;
    }
}
