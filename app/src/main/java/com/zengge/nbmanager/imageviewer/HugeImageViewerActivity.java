package com.zengge.nbmanager.imageviewer;

import android.graphics.Point;
import android.net.Uri;
import android.os.Bundle;
import android.view.Window;

import androidx.appcompat.app.AppCompatActivity;

import com.zengge.nbmanager.R;

import java.io.File;

public class HugeImageViewerActivity extends AppCompatActivity {

    private TileDrawable mTileDrawable;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_huge);
        final String str = this.getIntent().getStringExtra("IMAGEPATH");
        final PinchImageView pinchImageView = (PinchImageView) findViewById(R.id.pic);
        pinchImageView.post(() -> {
            mTileDrawable = new TileDrawable();
            mTileDrawable.setInitCallback(() -> pinchImageView.setImageDrawable(mTileDrawable));
            mTileDrawable.init(new HugeImageRegionLoader(HugeImageViewerActivity.this, Uri.fromFile(new File(str))), new Point(pinchImageView.getWidth(), pinchImageView.getHeight()));
        });
    }

    @Override
    protected void onDestroy() {
        if (mTileDrawable != null)
            mTileDrawable.recycle();
        super.onDestroy();
    }
}