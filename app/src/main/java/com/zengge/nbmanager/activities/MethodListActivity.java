package com.zengge.nbmanager.activities;

import android.content.Context;
import android.content.Intent;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.data.ActResConstant;
import com.zengge.nbmanager.R;
import com.zengge.nbmanager.dalvik.Parser;

import org.jetbrains.annotations.NotNull;
import org.jf.dexlib.ClassDataItem;
import org.jf.dexlib.ClassDataItem.EncodedMethod;
import org.jf.dexlib.ClassDefItem;

import java.util.ArrayList;
import java.util.List;

public class MethodListActivity extends AppCompatActivity {

    public static boolean isDirectMethod = true;
    public static int methodIndex;
    public ListView lv;
    private MethodListAdapter mAdapter;
    private List<String> methodList = new ArrayList<String>();
    private List<String> methodDescriptor = new ArrayList<String>();
    private int directMethodsCount;
    private ClassDefItem classDef;
    private int listPos;

    /*
     * private void search() { LayoutInflater inflate=getLayoutInflater();
     * ScrollView
     * scroll=(ScrollView)inflate.inflate(R.layout.alert_dialog_search_methods
     * ,null); final EditText srcName =
     * (EditText)scroll.findViewById(R.id.src_edit); final EditText dstName =
     * (EditText)scroll.findViewById(R.id.dst_edit); srcName.setText("");
     * dstName.setText("");
     *
     * AlertDialog.Builder alert = new AlertDialog.Builder(this);
     * alert.setTitle(R.string.search);
     *
     * RadioGroup radioGroup=(RadioGroup)scroll.findViewById(R.id.radio_group);
     * final RadioButton
     * rbtnString=(RadioButton)radioGroup.findViewById(R.id.search_string);
     * final RadioButton
     * rbtnMethod=(RadioButton)radioGroup.findViewById(R.id.search_method);
     * final RadioButton
     * rbtnField=(RadioButton)radioGroup.findViewById(R.id.search_field);
     *
     * //default rbtnString.setChecked(true);
     *
     *
     * alert.setView(scroll); alert.setPositiveButton(R.string.btn_ok, new
     * DialogInterface.OnClickListener() { public void onClick(DialogInterface
     * dialog, int whichButton) { String src = srcName.getText().toString(); if
     * (src.length() == 0) { toast("Search name is empty."); return; }
     * List<String> classList=new ArrayList<String>();
     * if(rbtnString.isChecked()){ searchString(src); } /* else
     * if(rbtnMethod.isChecked()){ String dst = dstName.getText().toString(); if
     * (dst.length() == 0) { toast("Type descriptor is empty."); return; }
     * searchMethodInClasses(classList,src,dst); }else
     * if(rbtnField.isChecked()){ String dst = dstName.getText().toString(); if
     * (dst.length() == 0) { toast("Type descriptor is empty."); return; }
     * searchFieldInClasses(classList,src,dst); } } });
     * alert.setNegativeButton(R.string.btn_cancel,null);
     *
     * alert.show(); }
     */
    public static void setMethodIndex(boolean isDirectMethod, int methodIndex) {
        MethodListActivity.isDirectMethod = isDirectMethod;
        MethodListActivity.methodIndex = methodIndex;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        init();
        setContentView(R.layout.listact);
        lv = findViewById(R.id.zglist);
        mAdapter = new MethodListAdapter(this);
        mAdapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onInvalidated() {
                init();
            }
        });
        lv.setAdapter(mAdapter);
        registerForContextMenu(lv);
        lv.setOnItemClickListener((list, view, position, id) -> {
            if (position < directMethodsCount) {
                isDirectMethod = true;
                methodIndex = position;
            } else {
                isDirectMethod = false;
                methodIndex = position - directMethodsCount;
            }
            listPos = list.getFirstVisiblePosition();
            startCodeEditor();
        });
    }

    private void startCodeEditor() {
        Intent intent = new Intent(this, CodeEditorActivity.class);
        startActivityForResult(intent, ActResConstant.method_list_item);
    }

    private void init() {
        if (methodList == null)
            methodList = new ArrayList<>();
        else
            methodList.clear();
        if (methodDescriptor == null)
            methodDescriptor = new ArrayList<>();
        else
            methodDescriptor.clear();
        classDef = ClassListActivity.curClassDef;
        ClassDataItem classData = classDef.getClassData();
        if (classData != null) {
            EncodedMethod[] directMethods = classData.getDirectMethods();
            directMethodsCount = directMethods.length;
            EncodedMethod[] virtualMethods = classData.getVirtualMethods();
            for (EncodedMethod method : directMethods) {
                methodList.add(method.method.getMethodName().getStringValue());
                methodDescriptor.add(method.method.getPrototype()
                        .getPrototypeString());
            }
            for (EncodedMethod method : virtualMethods) {
                methodList.add(method.method.getMethodName().getStringValue());
                methodDescriptor.add(method.method.getPrototype()
                        .getPrototypeString());
            }
        }
    }

    private void searchString() {
        LayoutInflater inflate = getLayoutInflater();
        ScrollView scroll = (ScrollView) inflate.inflate(
                R.layout.alert_dialog_search_string, null);
        final AppCompatEditText srcName = (AppCompatEditText) scroll.findViewById(R.id.src_edit);
        srcName.setText(ClassListActivity.searchString);
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.search_string);
        alert.setView(scroll);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> {
                    ClassListActivity.searchString = srcName.getText()
                            .toString();
                    if (ClassListActivity.searchString.length() == 0) {
                        toast(getString(R.string.search_name_empty));
                        return;
                    }
                    searchStringInMethods(ClassListActivity.searchString);
                });
        alert.setNegativeButton(R.string.btn_cancel, null);
        alert.show();
    }

    private void searchMethod() {
        LayoutInflater inflate = getLayoutInflater();
        ScrollView scroll = (ScrollView) inflate.inflate(
                R.layout.alert_dialog_search_method, null);
        final AppCompatEditText methodClass = scroll
                .findViewById(R.id.class_edit);
        final AppCompatCheckBox ignoreNameAndDescriptor = scroll
                .findViewById(R.id.ignore_name_descriptor);
        final AppCompatEditText methodName = scroll
                .findViewById(R.id.name_edit);
        final AppCompatEditText methodDescriptor = scroll
                .findViewById(R.id.descriptor_edit);
        final AppCompatCheckBox ignoreDescriptor = scroll
                .findViewById(R.id.ignore_descriptor);
        methodClass.setText(ClassListActivity.searchMethodClass);
        methodName.setText(ClassListActivity.searchMethodName);
        methodDescriptor.setText(ClassListActivity.searchMethodDescriptor);
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.search_method);
        alert.setView(scroll);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> {
                    ClassListActivity.searchMethodClass = methodClass
                            .getText().toString();
                    ClassListActivity.searchMethodName = methodName
                            .getText().toString();
                    ClassListActivity.searchMethodDescriptor = methodDescriptor
                            .getText().toString();
                    searchMethodInMethods(
                            ClassListActivity.searchMethodClass,
                            ClassListActivity.searchMethodName,
                            ClassListActivity.searchMethodDescriptor,
                            ignoreNameAndDescriptor.isChecked(),
                            ignoreDescriptor.isChecked());
                });
        alert.setNegativeButton(R.string.btn_cancel, null);
        alert.show();
    }

    private void searchField() {
        LayoutInflater inflate = getLayoutInflater();
        ScrollView scroll = (ScrollView) inflate.inflate(
                R.layout.alert_dialog_search_field, null);
        final AppCompatEditText fieldClass = scroll
                .findViewById(R.id.class_edit);
        final AppCompatCheckBox ignoreNameAndDescriptor = scroll
                .findViewById(R.id.ignore_name_descriptor);
        final AppCompatEditText fieldName = scroll
                .findViewById(R.id.name_edit);
        final AppCompatCheckBox ignoreDescriptor = scroll
                .findViewById(R.id.ignore_descriptor);
        final AppCompatEditText fieldDescriptor = scroll
                .findViewById(R.id.descriptor_edit);
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.search_field);
        alert.setView(scroll);
        fieldClass.setText(ClassListActivity.searchFieldClass);
        fieldName.setText(ClassListActivity.searchFieldName);
        fieldDescriptor.setText(ClassListActivity.searchFieldDescriptor);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> {
                    ClassListActivity.searchFieldClass = fieldClass
                            .getText().toString();
                    ClassListActivity.searchFieldName = fieldName.getText()
                            .toString();
                    ClassListActivity.searchFieldDescriptor = fieldDescriptor
                            .getText().toString();
                    searchFieldInMethods(
                            ClassListActivity.searchFieldClass,
                            ClassListActivity.searchFieldName,
                            ClassListActivity.searchFieldDescriptor,
                            ignoreNameAndDescriptor.isChecked(),
                            ignoreDescriptor.isChecked());
                });
        alert.setNegativeButton(R.string.btn_cancel, null);
        alert.show();
    }

    private void searchStringInMethods(String src) {
        ClassDataItem classData = classDef.getClassData();
        List<String> methodList = new ArrayList<>(1);
        List<Boolean> isDirectes = new ArrayList<>(1);
        List<Integer> methodIndexes = new ArrayList<>(1);
        if (classData != null) {
            EncodedMethod[] directMethods = classData.getDirectMethods();
            EncodedMethod[] virtualMethods = classData.getVirtualMethods();
            for (int i = 0, len = directMethods.length; i < len; i++) {
                EncodedMethod method = directMethods[i];
                if (Parser.searchStringInMethod(method, src)) {
                    methodList.add(this.methodList.get(i));
                    isDirectes.add(true);
                    methodIndexes.add(i);
                }
            }
            for (int i = 0, len = virtualMethods.length; i < len; i++) {
                EncodedMethod method = virtualMethods[i];
                if (Parser.searchStringInMethod(method, src)) {
                    methodList.add(this.methodList.get(i));
                    isDirectes.add(false);
                    methodIndexes.add(i);
                }
            }
        }
        SearchMethodsActivity.initMethodList(methodList, isDirectes,
                methodIndexes);
        sendIntentToSearchActivity();
    }

    private void searchMethodInMethods(String classType, String name,
                                       String descriptor, boolean ignoreNameAndDescriptor,
                                       boolean ignoreDescriptor) {
        ClassDataItem classData = classDef.getClassData();
        List<String> methodList = new ArrayList<>(1);
        List<Boolean> isDirectes = new ArrayList<>(1);
        List<Integer> methodIndexes = new ArrayList<>(1);
        if (classData != null) {
            EncodedMethod[] directMethods = classData.getDirectMethods();
            EncodedMethod[] virtualMethods = classData.getVirtualMethods();
            for (int i = 0, len = directMethods.length; i < len; i++) {
                EncodedMethod method = directMethods[i];
                if (Parser.searchMethodInMethod(method, classType, name,
                        descriptor, ignoreNameAndDescriptor, ignoreDescriptor)) {
                    methodList.add(this.methodList.get(i));
                    isDirectes.add(true);
                    methodIndexes.add(i);
                }
            }
            for (int i = 0, len = virtualMethods.length; i < len; i++) {
                EncodedMethod method = virtualMethods[i];
                if (Parser.searchMethodInMethod(method, classType, name,
                        descriptor, ignoreNameAndDescriptor, ignoreDescriptor)) {
                    methodList.add(this.methodList.get(i));
                    isDirectes.add(false);
                    methodIndexes.add(i);
                }
            }
        }
        SearchMethodsActivity.initMethodList(methodList, isDirectes,
                methodIndexes);
        sendIntentToSearchActivity();
    }

    private void searchFieldInMethods(String classType, String name,
                                      String descriptor, boolean ignoreNameAndDescriptor,
                                      boolean ignoreDescriptor) {
        ClassDataItem classData = classDef.getClassData();
        List<String> methodList = new ArrayList<>(1);
        List<Boolean> isDirectes = new ArrayList<>(1);
        List<Integer> methodIndexes = new ArrayList<>(1);
        if (classData != null) {
            EncodedMethod[] directMethods = classData.getDirectMethods();
            EncodedMethod[] virtualMethods = classData.getVirtualMethods();
            for (int i = 0, len = directMethods.length; i < len; i++) {
                EncodedMethod method = directMethods[i];
                if (Parser.searchFieldInMethod(method, classType, name,
                        descriptor, ignoreNameAndDescriptor, ignoreDescriptor)) {
                    methodList.add(this.methodList.get(i));
                    isDirectes.add(true);
                    methodIndexes.add(i);
                }
            }
            for (int i = 0, len = virtualMethods.length; i < len; i++) {
                EncodedMethod method = virtualMethods[i];
                if (Parser.searchFieldInMethod(method, classType, name,
                        descriptor, ignoreNameAndDescriptor, ignoreDescriptor)) {
                    methodList.add(this.methodList.get(i));
                    isDirectes.add(false);
                    methodIndexes.add(i);
                }
            }
        }
        SearchMethodsActivity.initMethodList(methodList, isDirectes,
                methodIndexes);
        sendIntentToSearchActivity();
    }

    private void sendIntentToSearchActivity() {
        Intent intent = new Intent(MethodListActivity.this,
                SearchMethodsActivity.class);
        startActivity(intent);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater in = getMenuInflater();
        in.inflate(R.menu.method_list_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(@NotNull MenuItem mi) {
        int id = mi.getItemId();
        switch (id) {
            case R.id.add_method:
                Intent intent = new Intent(this, MethodItemNewActivity.class);
                startActivityForResult(intent, ActResConstant.method_list_item);
                break;
            case R.id.search_string:
                searchString();
                break;
            case R.id.search_method:
                searchMethod();
                break;
            case R.id.search_field:
                searchField();
                break;
        }
        return true;
    }

    @Override
    public void onCreateContextMenu(@NotNull ContextMenu menu, View v,
                                    ContextMenuInfo menuInfo) {
        menu.add(Menu.NONE, R.string.remove_method, Menu.NONE,
                R.string.remove_method);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == ActResConstant.method_list_item) {
            switch (resultCode) {
                case ActResConstant.code_editor:
                case ActResConstant.add_method:
                    mAdapter.notifyDataSetInvalidated();
                    lv.setSelection(listPos);
            }
        }
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
        int position = info.position;
        if (item.getItemId() == R.string.remove_method) {
            ClassDataItem classData = classDef.getClassData();
            if (position < directMethodsCount)
                classData.removeDirectMethod(position);
            else
                classData.removeVirtualMethod(position - directMethodsCount);
            mAdapter.notifyDataSetInvalidated();
        }
        return true;
    }

    public void toast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (classDef.getClassData() != null)
                classDef.getClassData().sortMethods();
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        clearAll();
    }

    private void clearAll() {
        classDef = null;
        methodList = null;
        methodDescriptor = null;
        mAdapter = null;
        System.gc();
    }

    private class MethodListAdapter extends BaseAdapter {

        protected final Context mContext;
        protected final LayoutInflater mInflater;

        public MethodListAdapter(Context context) {
            mContext = context;
            mInflater = (LayoutInflater) mContext
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        public int getCount() {
            return methodList.size();
        }

        public Object getItem(int position) {
            return methodList.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            RelativeLayout container;
            if (convertView == null) {
                container = (RelativeLayout) mInflater.inflate(
                        R.layout.method_list_item, null);
            } else
                container = (RelativeLayout) convertView;
            AppCompatImageView icon = container.findViewById(R.id.icon);
            icon.setImageResource(R.drawable.ic_method);
            AppCompatTextView text = container.findViewById(R.id.text);
            text.setText(methodList.get(position));
            AppCompatTextView descriptor = container
                    .findViewById(R.id.descriptor);
            descriptor.setText(methodDescriptor.get(position));
            return container;
        }
    }
}