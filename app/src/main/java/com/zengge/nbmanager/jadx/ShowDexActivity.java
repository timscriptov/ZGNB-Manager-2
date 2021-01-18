package com.zengge.nbmanager.jadx;

import android.os.Bundle;
import android.view.ViewGroup;

import androidx.appcompat.app.AppCompatActivity;

import com.unnamed.b.atv.model.TreeNode;
import com.unnamed.b.atv.view.AndroidTreeView;
import com.zengge.nbmanager.R;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class ShowDexActivity extends AppCompatActivity {

    private ViewGroup containerView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.fragment_selectable_nodes);
        containerView = findViewById(R.id.container);
        TreeNode root = TreeNode.root();
        TreeNode parent = new TreeNode("MyParentNode");
        TreeNode child0 = new TreeNode("ChildNode0");
        TreeNode child1 = new TreeNode("ChildNode1");
        parent.addChildren(child0, child1);
        root.addChild(parent);
        AndroidTreeView tView = new AndroidTreeView(this, root);
        tView.setDefaultAnimation(true);
        containerView.addView(tView.getView());
        any();

    }

    public void any() {
        String file[] = new File("cache").list();
        String path = getFilesDir().getPath();
        List<String> strs = new ArrayList<>();
        strs.add(getPackageResourcePath());
    }
}
