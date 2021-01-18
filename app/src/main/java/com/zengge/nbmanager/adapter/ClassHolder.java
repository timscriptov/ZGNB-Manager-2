package com.zengge.nbmanager.adapter;

import android.annotation.SuppressLint;
import android.view.View;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.R;
import com.zengge.nbmanager.adapter.base.RecyclerViewAdapter;
import com.zengge.nbmanager.adapter.base.RecyclerViewHolder;

import org.jetbrains.annotations.NotNull;

import com.zengge.nbmanager.jadx.gui.treemodel.JNode;
import com.zengge.nbmanager.jadx.gui.treemodel.JPackage;

public class ClassHolder extends RecyclerViewHolder<ClassHolder> {

    AppCompatImageView fileIcon;
    AppCompatTextView fileName;
    AppCompatTextView fileChildCount;
    AppCompatTextView fileSize;
    AppCompatImageView dir_enter_image;

    public ClassHolder(View view) {
        super(view);
        fileIcon = view.findViewById(R.id.fileIcon);
        fileName = view.findViewById(R.id.fileName);
        fileChildCount = view.findViewById(R.id.fileChildCount);
        fileSize = view.findViewById(R.id.fileSize);
        dir_enter_image = view.findViewById(R.id.dir_enter_image);
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onBindViewHolder(final @NotNull ClassHolder classHolder, @NotNull RecyclerViewAdapter adapter, int position) {
        JNode jNode = (JNode) adapter.getItem(position);
        classHolder.fileName.setText(jNode.getName());

        if (R.drawable.ic_folder == jNode.getIcon()) {
            JPackage jPackage = (JPackage) jNode;
            classHolder.fileChildCount.setVisibility(View.VISIBLE);
            classHolder.fileChildCount.setText(jPackage.getClasses().size() + jPackage.getInnerPackages().size() + "项");
            classHolder.fileSize.setVisibility(View.GONE);
            classHolder.dir_enter_image.setVisibility(View.VISIBLE);

        } else {
            classHolder.fileChildCount.setVisibility(View.GONE);
            classHolder.fileSize.setVisibility(View.GONE);
            classHolder.dir_enter_image.setVisibility(View.GONE);
        }

        //设置图标
        classHolder.fileIcon.setImageResource(jNode.getIcon());
    }
}