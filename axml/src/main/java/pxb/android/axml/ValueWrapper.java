package pxb.android.axml;

import org.jetbrains.annotations.Contract;
import org.jetbrains.annotations.NotNull;

public class ValueWrapper {

    static final int ID = 1;
    static final int STYLE = 2;
    static final int CLASS = 3;
    public final int type;
    public final String raw;
    final int ref;

    private ValueWrapper(int type, int ref, String raw) {
        super();
        this.type = type;
        this.raw = raw;
        this.ref = ref;
    }

    @Contract("_, _ -> new")
    static @NotNull ValueWrapper wrapId(int ref, String raw) {
        return new ValueWrapper(ID, ref, raw);
    }

    @Contract("_, _ -> new")
    static @NotNull ValueWrapper wrapStyle(int ref, String raw) {
        return new ValueWrapper(STYLE, ref, raw);
    }

    @Contract("_, _ -> new")
    static @NotNull ValueWrapper wrapClass(int ref, String raw) {
        return new ValueWrapper(CLASS, ref, raw);
    }

    public ValueWrapper replaceRaw(String raw) {
        return new ValueWrapper(type, ref, raw);
    }
}