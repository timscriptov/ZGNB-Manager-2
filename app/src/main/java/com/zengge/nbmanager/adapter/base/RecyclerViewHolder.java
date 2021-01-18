package com.zengge.nbmanager.adapter.base;

import android.view.View;

import androidx.recyclerview.widget.RecyclerView;

/**
 * Created by ${zhaoyanjun} on 2017/1/12.
 */

public abstract class RecyclerViewHolder<T> extends RecyclerView.ViewHolder {

    public RecyclerViewHolder(View itemView) {
        super(itemView);
    }

    public abstract void onBindViewHolder(T t, RecyclerViewAdapter adapter, int position);
}
