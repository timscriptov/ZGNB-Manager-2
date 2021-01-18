package com.zengge.nbmanager.activities;

import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.R;

import org.jetbrains.annotations.NotNull;

import java.util.ArrayList;
import java.util.List;

public class ClassItemActivity extends AppCompatActivity {

    public ListView lv;
    private ClassItemAdapter mAdapter;
    private List<String> classList;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.listact);
        lv = findViewById(R.id.zglist);
        classList = new ArrayList<>();
        classList.add("ClassInfo");
        classList.add("Fields");
        classList.add("Methods");
        mAdapter = new ClassItemAdapter(getApplication());
        lv.setAdapter(mAdapter);
        registerForContextMenu(lv);
        lv.setOnItemClickListener((list, view, position, id) -> {
            if (position == 1) {
                Intent intent = new Intent(ClassItemActivity.this, FieldListActivity.class);
                startActivity(intent);
            } else if (position == 2) {
                Intent intent = new Intent(ClassItemActivity.this, MethodListActivity.class);
                startActivity(intent);
            } else if (position == 0) {
                Intent intent = new Intent(ClassItemActivity.this, ClassInfoEditorActivity.class);
                startActivity(intent);
            }
        });
    }

    /*
     * @Override public boolean onCreateOptionsMenu(Menu m){ MenuInflater
     * in=getMenuInflater(); in.inflate(R.menu.zip_editor_menu,m); return true;
     * }
     */

    @Override
    public boolean onOptionsItemSelected(@NotNull MenuItem mi) {
        int id = mi.getItemId();
        switch (id) {
            case R.id.add_entry:
                break;
            case R.id.save_file:
        }
        return true;
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
                                    ContextMenuInfo menuInfo) {
        // menu.add(Menu.NONE, R, Menu.NONE, "Remove");
        // menu.add(Menu.NONE, EXTRACT, Menu.NONE, "Extract");
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        ProgressDialog dialog = new ProgressDialog(this);
        dialog.setMessage("abcd");
        dialog.setIndeterminate(true);
        dialog.setCancelable(false);
        return dialog;
    }

    @Override
    public boolean onContextItemSelected(@NotNull MenuItem item) {
        AdapterView.AdapterContextMenuInfo info;
        try {
            info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
        } catch (ClassCastException e) {
            Log.e(e.toString(), "Bad menuInfo");
            return false;
        }
        int id = item.getItemId();
        return true;
    }

    public void toast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyDown(keyCode, event);
    }

    private class ClassItemAdapter extends BaseAdapter {

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
            String file = classList.get(position);
            LinearLayout container = (LinearLayout) mInflater.inflate(
                    R.layout.list_item, null);
            AppCompatImageView icon = container
                    .findViewById(R.id.list_item_icon);
            switch (position) {
                case 0:
                    icon.setImageResource(R.drawable.ic_file);
                    break;
                case 1:
                    icon.setImageResource(R.drawable.ic_field);
                    break;
                case 2:
                    icon.setImageResource(R.drawable.ic_method);
                    break;
            }
            AppCompatTextView text = container
                    .findViewById(R.id.list_item_title);
            text.setText(file);
            return container;
        }
    }
}