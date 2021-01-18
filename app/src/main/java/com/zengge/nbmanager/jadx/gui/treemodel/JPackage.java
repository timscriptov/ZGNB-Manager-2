package com.zengge.nbmanager.jadx.gui.treemodel;

import com.zengge.nbmanager.R;

import org.jetbrains.annotations.NotNull;

import java.util.ArrayList;
import java.util.List;

import jadx.api.JavaClass;
import jadx.api.JavaPackage;

public class JPackage extends JNode implements Comparable<JPackage> {
    private static final long serialVersionUID = -4120718634156839804L;
    private final List<JClass> classes;
    private final List<JPackage> innerPackages = new ArrayList<>(1);
    private String name;

    public JPackage(@NotNull JavaPackage pkg) {
        this.name = pkg.getName();
        List<JavaClass> javaClasses = pkg.getClasses();
        this.classes = new ArrayList<>(javaClasses.size());
        for (JavaClass javaClass : javaClasses) {
            classes.add(new JClass(javaClass));
        }
        //update();
    }

    public JPackage(String name) {
        this.name = name;
        this.classes = new ArrayList<>(1);
    }

    public final void update() {
        //removeAllChildren();
        for (JPackage pkg : innerPackages) {
            pkg.update();
            //add(pkg);
        }
        for (JClass cls : classes) {
            cls.update();
            //add(cls);
        }
    }

    @Override
    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public List<JPackage> getInnerPackages() {
        return innerPackages;
    }

    public List<JClass> getClasses() {
        return classes;
    }

    @Override
    public int getIcon() {
        return R.drawable.ic_folder;
    }

    @Override
    public JClass getJParent() {
        return null;
    }

    @Override
    public int getLine() {
        return 0;
    }

    @Override
    public int compareTo(@NotNull JPackage o) {
        return name.compareTo(o.name);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        return name.equals(((JPackage) o).name);
    }

    @Override
    public int hashCode() {
        return name.hashCode();
    }

    @Override
    public String makeString() {
        return name;
    }

    @Override
    public String makeLongString() {
        return name;
    }
}
