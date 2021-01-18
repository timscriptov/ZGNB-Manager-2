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

public class ClassAdapter extends RecyclerViewAdapter {

    private Context context;
    private List<JNode> list;
    private LayoutInflater mLayoutInflater;

    public ClassAdapter(Context context, List<JNode> list) {
        this.context = context;
        this.list = list;
        mLayoutInflater = LayoutInflater.from(context);
    }

    @Override
    public RecyclerView.@NotNull ViewHolder onCreateViewHolder(@NotNull ViewGroup parent, int viewType) {
        View view;
        if (viewType == 0) {
            view = mLayoutInflater.inflate(R.layout.list_item_file, parent, false);
            return new ClassHolder(view);
        } else {
            view = mLayoutInflater.inflate(R.layout.list_item_line, parent, false);
            return new LineHolder(view);
        }
    }

    @Override
    public void onBindViewHolders(final RecyclerView.ViewHolder holder,
                                  final int position) {
        if (holder instanceof ClassHolder) {
            ClassHolder classHolder = (ClassHolder) holder;
            classHolder.onBindViewHolder(classHolder, this, position);
        } else if (holder instanceof LineHolder) {
            LineHolder lineHolder = (LineHolder) holder;
            lineHolder.onBindViewHolder(lineHolder, this, position);
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
    public int getItemViewType(int position) {
        return 0;
    }

    @Override
    public int getItemCount() {
        if (list != null) {
            return list.size();
        } else {
            return 0;
        }
    }

    public void refresh(List<JNode> list) {
        this.list = list;
        notifyDataSetChanged();
    }
}
