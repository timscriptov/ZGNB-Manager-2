package jadx.core.dex.nodes;

import com.android.dex.ClassDef;
import com.android.dex.Dex;
import com.zengge.jadx.utils.files.DexFile;

import org.jetbrains.annotations.NotNull;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import jadx.core.dex.info.ClassInfo;
import jadx.core.utils.exceptions.DecodeException;

public class DexNode {
    private final RootNode root;
    private final Dex dexBuf;
    private final DexFile file;

    private final List<ClassNode> classes = new ArrayList<>();
    private final Map<ClassInfo, ClassNode> clsMap = new HashMap<>();

    public DexNode(RootNode root, @NotNull DexFile input) {
        this.root = root;
        this.file = input;
        this.dexBuf = input.getDexBuf();
    }

    public void loadClasses() throws DecodeException {
        for (ClassDef cls : dexBuf.classDefs()) {
            ClassNode clsNode = new ClassNode(this, cls);
            classes.add(clsNode);
            clsMap.put(clsNode.getClassInfo(), clsNode);
        }
    }
}
