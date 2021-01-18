package com.zengge.nbmanager.adapter;

import android.view.View;

import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.R;
import com.zengge.nbmanager.adapter.base.RecyclerViewAdapter;
import com.zengge.nbmanager.adapter.base.RecyclerViewHolder;
import com.zengge.nbmanager.bean.TitlePath;

import org.jetbrains.annotations.NotNull;

/**
 * Created by ${zhaoyanjun} on 2017/1/12.
 */

public class TitleHolder extends RecyclerViewHolder<TitleHolder> {

    AppCompatTextView textView;

    public TitleHolder(View itemView) {
        super(itemView);

        textView = itemView.findViewById(R.id.title_Name);
    }

    @Override
    public void onBindViewHolder(@NotNull TitleHolder lineHolder, @NotNull RecyclerViewAdapter adapter, int position) {
        TitlePath titlePath = (TitlePath) adapter.getItem(position);
        lineHolder.textView.setText(titlePath.getNameState());
    }
}
