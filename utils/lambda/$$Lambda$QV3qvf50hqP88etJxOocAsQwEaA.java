package com.zengge.jadx.utils.lambda;

import org.jetbrains.annotations.Unmodifiable;

import java.util.function.Function;

import jadx.core.dex.nodes.ClassNode;

public final class $$Lambda$QV3qvf50hqP88etJxOocAsQwEaA implements Function {
    public static final $$Lambda$QV3qvf50hqP88etJxOocAsQwEaA INSTANCE = new $$Lambda$QV3qvf50hqP88etJxOocAsQwEaA();

    private $$Lambda$QV3qvf50hqP88etJxOocAsQwEaA() {
    }

    public final @Unmodifiable Object apply(Object obj) {
        return ((ClassNode) obj).getFullName();
    }
}