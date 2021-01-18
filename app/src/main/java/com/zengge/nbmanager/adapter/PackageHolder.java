package com.zengge.nbmanager.adapter;

import android.annotation.SuppressLint;
import android.view.View;

import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.R;
import com.zengge.nbmanager.adapter.base.RecyclerViewAdapter;
import com.zengge.nbmanager.adapter.base.RecyclerViewHolder;

import org.jetbrains.annotations.NotNull;

import com.zengge.nbmanager.jadx.gui.treemodel.JNode;

public class PackageHolder extends RecyclerViewHolder<PackageHolder> {

    AppCompatTextView textView;

    public PackageHolder(View itemView) {
        super(itemView);

        textView = itemView.findViewById(R.id.title_Name);
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onBindViewHolder(@NotNull PackageHolder lineHolder, @NotNull RecyclerViewAdapter adapter, int position) {
        JNode jPackage = (JNode) adapter.getItem(position);
        lineHolder.textView.setText(jPackage.getName() + ">");
    }
}
