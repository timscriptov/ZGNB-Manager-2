package com.zengge.nbmanager.adapter.base;

import android.view.View;

import androidx.recyclerview.widget.RecyclerView;

import com.zengge.nbmanager.adapter.FileAdapter;

import org.jetbrains.annotations.NotNull;

/**
 * Created by ${zhaoyanjun} on 2017/1/12.
 */

public abstract class RecyclerViewAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {


    public FileAdapter.OnItemClickListener onItemClickListener;
    public FileAdapter.OnItemLongClickListener onItemLongClickListener;

    public void setOnItemClickListener(FileAdapter.OnItemClickListener listener) {
        onItemClickListener = listener;
    }

    public void setOnItemLongClickListener(FileAdapter.OnItemLongClickListener listener) {
        onItemLongClickListener = listener;
    }

    @Override
    public void onBindViewHolder(final RecyclerView.@NotNull ViewHolder holder, int position) {
        holder.itemView.setOnClickListener(v -> {
            if (onItemClickListener != null) {
                int pos = holder.getLayoutPosition();
                onItemClickListener.onItemClick(v, holder, pos);
            }
        });

        holder.itemView.setOnLongClickListener(v -> {
            if (onItemLongClickListener != null) {
                int pos = holder.getLayoutPosition();
                return onItemLongClickListener.onItemLongClick(v, holder, pos);
            }
            return false;
        });

        onBindViewHolders(holder, position);
    }

    public abstract void onBindViewHolders(RecyclerView.ViewHolder holder, int position);

    public abstract Object getAdapterData();

    public abstract Object getItem(int positon);

    public interface OnItemClickListener {
        void onItemClick(View view, RecyclerView.ViewHolder viewHolder, int position);
    }

    public interface OnItemLongClickListener {
        boolean onItemLongClick(View view, RecyclerView.ViewHolder viewHolder, int position);
    }
}