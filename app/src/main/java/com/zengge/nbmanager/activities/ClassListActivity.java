package com.zengge.nbmanager.activities;

import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
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
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatCheckBox;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;

import com.zengge.nbmanager.data.ActResConstant;
import com.zengge.nbmanager.R;
import com.zengge.nbmanager.dalvik.Parser;

import org.jetbrains.annotations.NotNull;
import org.jf.dexlib.ClassDataItem;
import org.jf.dexlib.ClassDefItem;
import org.jf.dexlib.DexFile;
import org.jf.dexlib.IndexedSection;
import org.jf.dexlib.TypeIdItem;
import org.jf.dexlib.Util.ByteArrayAnnotatedOutput;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Stack;

public class ClassListActivity extends AppCompatActivity {

    public static final int SAVEFILE = 1;
    public static final int SAVEDISMISS = 2;
    public static final String MERGER = "MergerDexFile";
    private static final String title = "/";
    private static final int OPENDIR = 10;
    private static final int BACK = 11;
    private static final int UPDATE = 12;
    private static final int INIT = 13;
    private static final int TOAST = 14;
    private static final int SEARCH = 15;
    private static final int SEARCHDISMISS = 16;
    public static String searchString = "";
    public static String searchFieldClass = "";
    public static String searchFieldName = "";
    public static String searchFieldDescriptor = "";
    public static String searchMethodClass = "";
    public static String searchMethodName = "";
    public static String searchMethodDescriptor = "";
    public static HashMap<String, ClassDefItem> classMap;
    public static HashMap<String, ClassDefItem> deleteclassMap;
    public static DexFile dexFile;
    public static boolean isChanged;
    public static ClassDefItem curClassDef;
    public static String curFile;
    // tree dep
    private static int dep;
    private static Stack<String> path;
    public Tree tree;
    public ListView lv;
    private ClassListAdapter mAdapter;
    private List<String> classList;
    private int mod;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(@NotNull Message msg) {
            switch (msg.what) {
                case SAVEFILE:
                    showDialog(SAVEFILE);
                    break;
                case SEARCH:
                    showDialog(SEARCH);
                    break;
                case SAVEDISMISS:
                    dismissDialog(SAVEFILE);
                    break;
                case SEARCHDISMISS:
                    dismissDialog(SEARCH);
                    break;
                case TOAST:
                    toast(msg.obj.toString());
                    break;
            }
        }
    };

    public static void setCurrnetClass(String className) {
        curClassDef = classMap.get(className);
    }

    private static void searchStringInMethods(List<String> list, String src) {
        HashMap<String, ClassDefItem> classMap = ClassListActivity.classMap;
        HashMap<String, ClassDefItem> deleteclassMap = ClassListActivity.deleteclassMap;
        for (Map.Entry<String, ClassDefItem> entry : classMap.entrySet()) {
            if (deleteclassMap != null
                    && deleteclassMap.get(entry.getKey()) != null)
                continue;
            ClassDefItem classItem = entry.getValue();
            boolean isSearch = false;
            ClassDataItem classData = classItem.getClassData();
            if (classData != null) {
                //
                ClassDataItem.EncodedMethod[] methods = classData
                        .getDirectMethods();
                for (ClassDataItem.EncodedMethod method : methods) {
                    if (Parser.searchStringInMethod(method, src)) {
                        String name = classItem.getClassType()
                                .getTypeDescriptor();
                        list.add(name.substring(1, name.length() - 1));
                        isSearch = true;
                        break;
                    }
                }
                if (isSearch)
                    continue;
                // virtual methods
                methods = classData.getVirtualMethods();
                for (ClassDataItem.EncodedMethod method : methods) {
                    if (Parser.searchStringInMethod(method, src)) {
                        String name = classItem.getClassType()
                                .getTypeDescriptor();
                        list.add(name.substring(1, name.length() - 1));
                        break;
                    }
                }
            }
        }
    }

    private static void searchFieldInMethods(List<String> list,
                                             String classType, String name, String descriptor,
                                             boolean ignoreNameAndDescriptor, boolean ignoreDescriptor) {
        HashMap<String, ClassDefItem> classMap = ClassListActivity.classMap;
        HashMap<String, ClassDefItem> deleteclassMap = ClassListActivity.deleteclassMap;
        for (Map.Entry<String, ClassDefItem> entry : classMap.entrySet()) {
            if (deleteclassMap != null
                    && deleteclassMap.get(entry.getKey()) != null)
                continue;
            ClassDefItem classItem = entry.getValue();
            boolean isSearch = false;
            ClassDataItem classData = classItem.getClassData();
            if (classData != null) {
                //
                ClassDataItem.EncodedMethod[] methods = classData
                        .getDirectMethods();
                for (ClassDataItem.EncodedMethod method : methods) {
                    if (Parser.searchFieldInMethod(method, classType, name,
                            descriptor, ignoreNameAndDescriptor,
                            ignoreDescriptor)) {
                        String string = classItem.getClassType()
                                .getTypeDescriptor();
                        list.add(string.substring(1, string.length() - 1));
                        isSearch = true;
                        break;
                    }
                }
                if (isSearch)
                    continue;
                // virtual methods
                methods = classData.getVirtualMethods();
                for (ClassDataItem.EncodedMethod method : methods) {
                    if (Parser.searchFieldInMethod(method, classType, name,
                            descriptor, ignoreNameAndDescriptor,
                            ignoreDescriptor)) {
                        String string = classItem.getClassType()
                                .getTypeDescriptor();
                        list.add(string.substring(1, string.length() - 1));
                        break;
                    }
                }
            }
        }
    }

    private static void searchMethodInMethods(List<String> list,
                                              String classType, String name, String descriptor,
                                              boolean ignoreNameAndDescriptor, boolean ignoreDescriptor) {
        HashMap<String, ClassDefItem> classMap = ClassListActivity.classMap;
        HashMap<String, ClassDefItem> deleteclassMap = ClassListActivity.deleteclassMap;
        for (Map.Entry<String, ClassDefItem> entry : classMap.entrySet()) {
            if (deleteclassMap != null
                    && deleteclassMap.get(entry.getKey()) != null)
                continue;
            ClassDefItem classItem = entry.getValue();
            boolean isSearch = false;
            ClassDataItem classData = classItem.getClassData();
            if (classData != null) {
                //
                ClassDataItem.EncodedMethod[] methods = classData
                        .getDirectMethods();
                for (ClassDataItem.EncodedMethod method : methods) {
                    if (Parser.searchMethodInMethod(method, classType, name,
                            descriptor, ignoreNameAndDescriptor,
                            ignoreDescriptor)) {
                        String string = classItem.getClassType()
                                .getTypeDescriptor();
                        list.add(string.substring(1, string.length() - 1));
                        isSearch = true;
                        break;
                    }
                }
                if (isSearch)
                    continue;
                // virtual methods
                methods = classData.getVirtualMethods();
                for (ClassDataItem.EncodedMethod method : methods) {
                    if (Parser.searchMethodInMethod(method, classType, name,
                            descriptor, ignoreNameAndDescriptor,
                            ignoreDescriptor)) {
                        String string = classItem.getClassType()
                                .getTypeDescriptor();
                        list.add(string.substring(1, string.length() - 1));
                        break;
                    }
                }
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.class_list);
        lv = findViewById(R.id.clslist);
        init();
        mAdapter = new ClassListAdapter(this);
        mAdapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onInvalidated() {
                switch (mod) {
                    case OPENDIR:
                        tree.push(curFile);
                        classList = tree.list();
                        break;
                    case BACK:
                        tree.pop();
                        classList = tree.list();
                        break;
                    case UPDATE:
                        classList = tree.list();
                        break;
                    case INIT:
                        init();
                        break;
                }
                setTitle(title + tree.getCurPath());
            }
        });
        lv.setAdapter(mAdapter);
        registerForContextMenu(lv);
        lv.setOnItemClickListener((list, view, position, id) -> {
            curFile = (String) list.getItemAtPosition(position);
            if (tree.isDirectory(curFile)) {
                mod = OPENDIR;
                mAdapter.notifyDataSetInvalidated();
                return;
            }
            curClassDef = classMap.get(tree.getCurPath() + curFile);
            Intent intent = new Intent(ClassListActivity.this, ClassItemActivity.class);
            startActivity(intent);
        });
        AppCompatButton btn = findViewById(R.id.btn_string_pool);
        btn.setOnClickListener(v -> openStringPool());
    }

    private void init() {
        if (classMap == null)
            classMap = new HashMap<>();
        else
            classMap.clear();
        HashMap<String, ClassDefItem> classMap = ClassListActivity.classMap;
        HashMap<String, ClassDefItem> deleteclassMap = ClassListActivity.deleteclassMap;
        for (ClassDefItem classItem : dexFile.ClassDefsSection.getItems()) {
            String className = classItem.getClassType().getTypeDescriptor();
            className = className.substring(1, className.length() - 1);
            if (deleteclassMap != null && deleteclassMap.get(className) != null)
                continue;
            classMap.put(className, classItem);
        }
        tree = new Tree(classMap.keySet());
        setTitle(title + tree.getCurPath());
        classList = tree.list();
    }

    /*
     * @Override protected void onSaveInstanceState(Bundle status){ }
     */
    @Override
    public boolean onCreateOptionsMenu(Menu m) {
        MenuInflater in = getMenuInflater();
        in.inflate(R.menu.class_list_menu, m);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(@NotNull MenuItem mi) {
        int id = mi.getItemId();
        switch (id) {
            case R.id.save_dexfile:
                new Thread(() -> {
                    mHandler.sendEmptyMessage(SAVEFILE);
                    saveDexFile();
                    mHandler.sendEmptyMessage(SAVEDISMISS);
                    setResultToZipEditor();
                }).start();
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
            case R.id.merger_dexfile:
                selectDexFile();
                break;
        }
        return true;
    }

    @Override
    public void onCreateContextMenu(@NotNull ContextMenu menu, View v,
                                    ContextMenuInfo menuInfo) {
        menu.add(Menu.NONE, R.string.rename_class, Menu.NONE,
                R.string.rename_class);
        menu.add(Menu.NONE, R.string.remove_class, Menu.NONE,
                R.string.remove_class);
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == ActResConstant.class_list_item) {
            switch (resultCode) {
                case ActResConstant.add_entry:
                    if (mergerDexFile(data.getStringExtra(MainActivity.ENTRYPATH)))
                        toast(getString(R.string.dex_merged));
                    break;
            }
        }
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        ProgressDialog dialog = new ProgressDialog(this);
        switch (id) {
            case SAVEFILE:
                dialog.setMessage(getString(R.string.saving));
                break;
            case SEARCH:
                dialog.setMessage(getString(R.string.searching));
                break;
        }
        dialog.setIndeterminate(true);
        dialog.setCancelable(false);
        return dialog;
    }

    private void searchString() {
        LayoutInflater inflate = getLayoutInflater();
        ScrollView scroll = (ScrollView) inflate.inflate(
                R.layout.alert_dialog_search_string, null);
        final AppCompatEditText srcName = scroll.findViewById(R.id.src_edit);
        srcName.setText(searchString);
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.search_string);
        alert.setView(scroll);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> {
                    searchString = srcName.getText().toString();
                    if (searchString.length() == 0) {
                        toast(getString(R.string.search_name_empty));
                        return;
                    }
                    new Thread(() -> {
                        mHandler.sendEmptyMessage(SEARCH);
                        List<String> classList = new ArrayList<>();
                        searchStringInMethods(classList, searchString);
                        SearchClassesActivity.initClassList(classList);
                        mHandler.sendEmptyMessage(SEARCHDISMISS);
                        sendIntentToSearchActivity();
                    }).start();
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
        fieldClass.setText(searchFieldClass);
        fieldName.setText(searchFieldName);
        fieldDescriptor.setText(searchFieldDescriptor);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> new Thread(() -> {
                    mHandler.sendEmptyMessage(SEARCH);
                    searchFieldClass = fieldClass.getText()
                            .toString();
                    searchFieldName = fieldName.getText()
                            .toString();
                    searchFieldDescriptor = fieldDescriptor
                            .getText().toString();
                    List<String> classList = new ArrayList<String>();
                    searchFieldInMethods(classList,
                            searchFieldClass, searchFieldName,
                            searchFieldDescriptor,
                            ignoreNameAndDescriptor.isChecked(),
                            ignoreDescriptor.isChecked());
                    SearchClassesActivity.initClassList(classList);
                    mHandler.sendEmptyMessage(SEARCHDISMISS);
                    sendIntentToSearchActivity();
                }).start());
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
        final AppCompatCheckBox ignoreDescriptor = scroll
                .findViewById(R.id.ignore_descriptor);
        final AppCompatEditText methodDescriptor = scroll
                .findViewById(R.id.descriptor_edit);
        methodClass.setText(searchMethodClass);
        methodName.setText(searchMethodName);
        methodDescriptor.setText(searchMethodDescriptor);
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.search_method);
        alert.setView(scroll);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> {
                    searchMethodClass = methodClass.getText().toString();
                    searchMethodName = methodName.getText().toString();
                    searchMethodDescriptor = methodDescriptor.getText()
                            .toString();
                    List<String> classList = new ArrayList<>();
                    searchMethodInMethods(classList, searchMethodClass,
                            searchMethodName, searchMethodDescriptor,
                            ignoreNameAndDescriptor.isChecked(),
                            ignoreDescriptor.isChecked());
                    SearchClassesActivity.initClassList(classList);
                    sendIntentToSearchActivity();
                });
        alert.setNegativeButton(R.string.btn_cancel, null);
        alert.show();
    }

    private void sendIntentToSearchActivity() {
        Intent intent = new Intent(ClassListActivity.this,
                SearchClassesActivity.class);
        startActivity(intent);
    }

    private void clearAll() {
        if (classMap != null)
            classMap.clear();
        classMap = null;
        deleteclassMap = null;
        path = null;
        dexFile = null;
        curClassDef = null;
        tree = null;
        curFile = null;
        isChanged = false;
        System.gc();
    }

    private void saveDexFile() {
        DexFile outDexFile = new DexFile();
        HashMap<String, ClassDefItem> classMap = ClassListActivity.classMap;
        HashMap<String, ClassDefItem> deleteclassMap = ClassListActivity.deleteclassMap;
        for (Map.Entry<String, ClassDefItem> entry : classMap.entrySet()) {
            if (deleteclassMap != null
                    && deleteclassMap.get(entry.getKey()) != null)
                continue;
            ClassDefItem classDef = entry.getValue();
            classDef.internClassDefItem(outDexFile);
        }
        outDexFile.setSortAllItems(true);
        outDexFile.place();
        // out dex byte array
        byte[] buf = new byte[outDexFile.getFileSize()];
        ByteArrayAnnotatedOutput out = new ByteArrayAnnotatedOutput(buf);
        outDexFile.writeTo(out);
        DexFile.calcSignature(buf);
        DexFile.calcChecksum(buf);
        TextEditorActivity.data = buf;
        outDexFile = null;
        isChanged = false;
    }

    private boolean mergerDexFile(String name) {
        try {
            DexFile tmp = new DexFile(name);
            DexFile dexFile = ClassListActivity.dexFile;
            IndexedSection<ClassDefItem> classes = tmp.ClassDefsSection;
            List<ClassDefItem> classDefList = classes.getItems();
            for (ClassDefItem classDef : classDefList) {
                String className = classDef.getClassType().getTypeDescriptor();
                className = className.substring(1, className.length() - 1);
                if (deleteclassMap != null)
                    deleteclassMap.put(className, null);
                classDef.internClassDefItem(dexFile);
            }
            mod = INIT;
            mAdapter.notifyDataSetInvalidated();
            isChanged = true;
        } catch (Exception e) {
            MainActivity.showMessage(this, "Open dexFile exception",
                    e.getMessage());
            return false;
        }
        System.gc();
        return true;
    }

    private void openStringPool() {
        Intent intent = new Intent(this, TextEditorActivity.class);
        intent.putExtra(TextEditorActivity.PLUGIN, "StringIdsEditor");
        startActivity(intent);
    }

    private void replaceClassType(String src, String dst) {
        for (TypeIdItem type : dexFile.TypeIdsSection.getItems()) {
            String s = type.getTypeDescriptor();
            // skip start 'L'
            int pos = 1;
            for (int i = 0; i < s.length(); i++) {
                if (s.charAt(i) != '[')
                    break;
                pos++;
            }
            int i = s.indexOf(src);
            if (i != -1 && i == pos) {
                s = s.replace(src, dst);
                type.setTypeDescriptor(s);
            }
        }
    }

    private void renameType(final @NotNull String className) {
        final AppCompatEditText newName = new AppCompatEditText(this);
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        final boolean isDirectory = className.endsWith("/");
        if (isDirectory)
            newName.setText(className.substring(0, className.length() - 1));
        else
            newName.setText(className);
        alert.setTitle(R.string.rename);
        alert.setView(newName);
        alert.setPositiveButton(R.string.btn_ok,
                (dialog, whichButton) -> {
                    String name = newName.getText().toString();
                    if (name.length() == 0 || name.indexOf("/") != -1) {
                        toast(getString(R.string.name_empty));
                        return;
                    } else {
                        for (String s : classList) {
                            if (s.equals(name)) {
                                toast(String.format(
                                        getString(R.string.class_exists),
                                        name));
                                return;
                            }
                        }
                    }
                    name += isDirectory ? "/" : "";
                    String cur = tree.getCurPath();
                    replaceClassType(cur + className, cur + name);
                    isChanged = true;
                    mod = INIT;
                    mAdapter.notifyDataSetInvalidated();
                });
        alert.setNegativeButton(R.string.btn_cancel, null);
        alert.show();
    }

    private void selectDexFile() {
        Intent intent = new Intent(this, MainActivity.class);
        intent.putExtra(MainActivity.SELECTEDMOD, true);
        startActivityForResult(intent, ActResConstant.class_list_item);
    }

    private void showDialogIfChanged() {
        if (isChanged) {
            MainActivity.prompt(this, getString(R.string.prompt),
                    getString(R.string.is_save),
                    (dailog, which) -> {
                        if (which == AlertDialog.BUTTON_POSITIVE) {
                            new Thread(() -> {
                                mHandler.sendEmptyMessage(SAVEFILE);
                                saveDexFile();
                                mHandler.sendEmptyMessage(SAVEDISMISS);
                                setResultToZipEditor();
                            }).start();
                        } else if (which == AlertDialog.BUTTON_NEGATIVE)
                            finish();
                    });
        } else
            finish();
    }

    private void setResultToZipEditor() {
        Intent intent = getIntent();
        setResult(ActResConstant.text_editor, intent);
        finish();
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
        switch (item.getItemId()) {
            case R.string.rename_class: {
                String className = classList.get(info.position);
                renameType(className);
            }
            break;
            case R.string.remove_class:
                final String name = classList.get(info.position);
                MainActivity.prompt(this, getString(R.string.is_remove), name,
                        (dialog, which) -> {
                            if (which == AlertDialog.BUTTON_POSITIVE) {
                                if (tree.isDirectory(name))
                                    removeClassesDir(name);
                                else
                                    removeClasses(name);
                            }
                        });
                break;
        }
        return true;
    }

    private void removeClassesDir(String name) {
        if (deleteclassMap == null)
            deleteclassMap = new HashMap<>();
        HashMap<String, ClassDefItem> deleteclassMap = ClassListActivity.deleteclassMap;
        String cur = tree.getCurPath() + name;
        for (String key : classMap.keySet()) {
            if (key.indexOf(cur) == 0)
                deleteclassMap.put(key, classMap.get(key));
        }
        isChanged = true;
        mod = INIT;
        mAdapter.notifyDataSetInvalidated();
    }

    private void removeClasses(String name) {
        if (deleteclassMap == null)
            deleteclassMap = new HashMap<>();
        String cur = tree.getCurPath() + name;
        deleteclassMap.put(cur, classMap.get(cur));
        isChanged = true;
        mod = INIT;
        mAdapter.notifyDataSetInvalidated();
    }

    public void toast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        clearAll();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (!getTitle().equals(title)) {
                mod = BACK;
                mAdapter.notifyDataSetInvalidated();
                return true;
            } else {
                showDialogIfChanged();
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    static class Tree {
        List<Map<String, String>> node;
        Comparator<String> sortByType = (a, b) -> {
            if (isDirectory(a) && !isDirectory(b))
                return -1;
            if (!isDirectory(a) && isDirectory(b))
                return 1;
            return a.toLowerCase().compareTo(b.toLowerCase());
        };

        public Tree(Set<String> names) {
            if (path == null) {
                path = new Stack<>();
                dep = 0;
            }
            HashMap<String, ClassDefItem> classMap = ClassListActivity.classMap;
            node = new ArrayList<>();
            for (String name : names) {
                String[] token = name.split("/");
                String tmp = "";
                for (int i = 0, len = token.length; i < len; i++) {
                    String value = token[i];
                    if (i >= node.size()) {
                        Map<String, String> map = new HashMap<>();
                        if (classMap.containsKey(tmp + value) && i + 1 == len)
                            map.put(tmp + value, tmp);
                        else
                            map.put(tmp + value + "/", tmp);
                        node.add(map);
                        tmp += value + "/";
                    } else {
                        Map<String, String> map = node.get(i);
                        if (classMap.containsKey(tmp + value) && i + 1 == len)
                            map.put(tmp + value, tmp);
                        else
                            map.put(tmp + value + "/", tmp);
                        tmp += value + "/";
                    }
                }
            }
        }

        private @NotNull List<String> list(String parent) {
            Map<String, String> map = null;
            List<String> str = new ArrayList<String>();
            while (dep >= 0 && node.size() > 0) {
                map = node.get(dep);
                if (map != null)
                    break;
                pop();
            }
            if (map == null)
                return str;
            for (String key : map.keySet()) {
                if (parent.equals(map.get(key))) {
                    int index;
                    if (key.endsWith("/"))
                        index = key.lastIndexOf("/", key.length() - 2);
                    else
                        index = key.lastIndexOf("/");
                    if (index != -1)
                        key = key.substring(index + 1);
                    str.add(key);
                    // Log.e("tree",key);
                }
            }
            Collections.sort(str, sortByType);
            return str;
        }

        public void addNode(String name) {
            Map<String, String> map = node.get(dep);
            map.put(getCurPath() + name, getCurPath());
        }

        public void deleteNode(String name) {
            Map<String, String> map = node.get(dep);
            map.remove(getCurPath() + name);
        }

        public List<String> list() {
            return list(getCurPath());
        }

        public void push(String name) {
            dep++;
            path.push(name);
        }

        public String pop() {
            if (dep > 0) {
                dep--;
                return path.pop();
            }
            return null;
        }

        public String getCurPath() {
            return join(path, "/");
        }

        public boolean isDirectory(@NotNull String name) {
            return name.endsWith("/");
        }

        private @NotNull String join(@NotNull Stack<String> stack, String d) {
            StringBuilder sb = new StringBuilder("");
            for (String s : stack)
                sb.append(s);
            return sb.toString();
        }

    }

    private class ClassListAdapter extends BaseAdapter {

        protected final Context mContext;
        protected final LayoutInflater mInflater;
        LinearLayout container;

        public ClassListAdapter(Context context) {
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
            if (convertView == null) {
                container = (LinearLayout) mInflater.inflate(
                        R.layout.class_list_item, null);
            } else
                container = (LinearLayout) convertView;
            AppCompatImageView icon = container
                    .findViewById(R.id.list_item_icon);
            if (tree.isDirectory(file))
                icon.setImageResource(R.drawable.ic_folder);
            else
                icon.setImageResource(R.drawable.ic_class);
            AppCompatTextView text = container
                    .findViewById(R.id.list_item_title);
            text.setText(file);
            return container;
        }
    }
}