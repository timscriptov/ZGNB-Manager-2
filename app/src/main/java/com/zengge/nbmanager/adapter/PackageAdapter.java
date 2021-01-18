package com.zengge.nbmanager.adapter;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.recyclerview.widget.RecyclerView;

import com.zengge.nbmanager.R;
import com.zengge.nbmanager.adapter.base.RecyclerViewAdapter;

import org.jetbrains.annotations.NotNull;

import java.util.List;

import com.zengge.nbmanager.jadx.gui.treemodel.JNode;

public class PackageAdapter extends RecyclerViewAdapter {
    private List<JNode> list;
    private LayoutInflater mLayoutInflater;

    public PackageAdapter(Context context, List<JNode> list) {
        this.list = list;
        mLayoutInflater = LayoutInflater.from(context);
    }

    @Override
    public RecyclerView.@NotNull ViewHolder onCreateViewHolder(@NotNull ViewGroup parent, int viewType) {
        View view = mLayoutInflater.inflate(R.layout.title_holder, parent, false);
        return new PackageHolder(view);
    }

    @Override
    public void onBindViewHolders(RecyclerView.ViewHolder holder, int position) {
        if (holder instanceof PackageHolder) {
            PackageHolder titleHolder = (PackageHolder) holder;
            titleHolder.onBindViewHolder(titleHolder, this, position);
        }
    }

    @Override
    public Object getAdapterData() {
        return list;
    }

    @Override
    public Object getItem(int positon) {
        return list.get(positon);
    }

    @Override
    public int getItemCount() {
        if (list == null) return 0;
        return list.size();
    }

    public void addItem(JNode jPackage) {
        list.add(jPackage);
        notifyItemChanged(list.size() - 1);
    }

    public void removeItem(int positon) {
        list.remove(positon);
        notifyItemRemoved(positon);
    }

    public void removeLast() {
        if (list == null) return;
        int lastPosition = getItemCount() - 1;
        list.remove(lastPosition);
        notifyItemRemoved(lastPosition);
    }
}
