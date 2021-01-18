package com.zengge.nbmanager.adapter;

import android.annotation.SuppressLint;
import android.view.View;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;

import com.bumptech.glide.Glide;
import com.zengge.nbmanager.R;
import com.zengge.nbmanager.adapter.base.RecyclerViewAdapter;
import com.zengge.nbmanager.adapter.base.RecyclerViewHolder;
import com.zengge.nbmanager.bean.FileBean;
import com.zengge.nbmanager.bean.FileType;
import com.zengge.nbmanager.utils.DecompileFile;

import org.jetbrains.annotations.NotNull;

import java.io.File;

/**
 * Created by ${zhaoyanjun} on 2017/1/12.
 */

public class FileHolder extends RecyclerViewHolder<FileHolder> {
    AppCompatImageView fileIcon;
    AppCompatTextView fileName;
    AppCompatTextView fileChildCount;
    AppCompatTextView fileSize;
    AppCompatImageView dir_enter_image;

    public FileHolder(View view) {
        super(view);
        fileIcon = view.findViewById(R.id.fileIcon);
        fileName = view.findViewById(R.id.fileName);
        fileChildCount = view.findViewById(R.id.fileChildCount);
        fileSize = view.findViewById(R.id.fileSize);
        dir_enter_image = view.findViewById(R.id.dir_enter_image);
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onBindViewHolder(final @NotNull FileHolder fileHolder, @NotNull RecyclerViewAdapter adapter, int position) {
        FileBean fileBean = (FileBean) adapter.getItem(position);
        fileHolder.fileName.setText(fileBean.getName());

        FileType fileType = fileBean.getFileType();
        if (fileType == FileType.directory) {
            fileHolder.fileChildCount.setVisibility(View.VISIBLE);
            fileHolder.fileChildCount.setText(fileBean.getChildCount() + "项");

            fileHolder.fileSize.setVisibility(View.GONE);
            fileHolder.dir_enter_image.setVisibility(View.VISIBLE);

        } else {
            fileHolder.fileChildCount.setVisibility(View.GONE);

            fileHolder.fileSize.setVisibility(View.VISIBLE);
            fileHolder.fileSize.setText(DecompileFile.sizeToChange(fileBean.getSize()));

            fileHolder.dir_enter_image.setVisibility(View.GONE);
        }

        //设置图标
        if (fileType == FileType.directory) {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_dir);
        } else if (fileType == FileType.music) {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_music);
        } else if (fileType == FileType.video) {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_video);
        } else if (fileType == FileType.txt) {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_txt);
        } else if (fileType == FileType.zip) {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_zip);
        } else if (fileType == FileType.image) {
            Glide.with(fileHolder.itemView.getContext()).load(new File(fileBean.getPath())).into(fileHolder.fileIcon);
        } else if (fileType == FileType.apk) {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_apk);
        } else {
            fileHolder.fileIcon.setImageResource(R.mipmap.file_icon_other);
        }
    }
}