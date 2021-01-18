package com.zengge.nbmanager.activities;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.R;

import java.util.ArrayList;
import java.util.List;

public class SearchClassesActivity extends AppCompatActivity {

    public static List<String> classList;
    public ListView lv;
    private ClassItemAdapter mAdapter;

    public static void initClassList(List<String> list) {
        classList = list;
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.listact);
        lv = findViewById(R.id.zglist);
        if (classList == null)
            classList = new ArrayList<String>();
        mAdapter = new ClassItemAdapter(getApplication());
        lv.setAdapter(mAdapter);
        lv.setOnItemClickListener((list, view, position, id) -> {
            ClassListActivity.setCurrnetClass(classList.get(position));
            Intent intent = new Intent(SearchClassesActivity.this, ClassItemActivity.class);
            startActivity(intent);
        });
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        clearAll();
    }

    private void clearAll() {
        classList = null;
        mAdapter = null;
    }

    private static class ClassItemAdapter extends BaseAdapter {

        protected final Context mContext;
        protected final LayoutInflater mInflater;

        public ClassItemAdapter(Context context) {
            mContext = context;
            mInflater = (LayoutInflater) mContext
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        public int getCount() {
            return classList.size();
        }

        public Object getItem(int position) {
            return classList.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            LinearLayout container;
            if (convertView == null) {
                container = (LinearLayout) mInflater.inflate(
                        R.layout.list_item, null);
            } else
                container = (LinearLayout) convertView;
            AppCompatImageView icon = container
                    .findViewById(R.id.list_item_icon);
            icon.setImageResource(R.drawable.ic_class);
            AppCompatTextView text = container
                    .findViewById(R.id.list_item_title);
            text.setText(classList.get(position));
            return container;
        }
    }
}