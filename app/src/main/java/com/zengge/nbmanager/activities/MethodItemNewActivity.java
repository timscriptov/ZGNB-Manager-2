package com.zengge.nbmanager.activities;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatEditText;

import com.zengge.nbmanager.data.ActResConstant;
import com.zengge.nbmanager.R;
import com.zengge.nbmanager.dalvik.Parser;

import org.jf.dexlib.ClassDataItem.EncodedMethod;
import org.jf.dexlib.ClassDefItem;
import org.jf.dexlib.Code.Format.Instruction10x;
import org.jf.dexlib.Code.Instruction;
import org.jf.dexlib.Code.Opcode;
import org.jf.dexlib.CodeItem;
import org.jf.dexlib.DexFile;
import org.jf.dexlib.MethodIdItem;
import org.jf.dexlib.ProtoIdItem;
import org.jf.dexlib.StringIdItem;
import org.jf.dexlib.TypeIdItem;
import org.jf.dexlib.TypeListItem;
import org.jf.dexlib.Util.AccessFlags;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public class MethodItemNewActivity extends AppCompatActivity {
    public static final Pattern pattern = Pattern.compile("\\s");
    public static final Pattern pParams = Pattern.compile("\\s|\\(|\\)");
    private boolean isChanged;
    private AppCompatEditText accessFlagsEdit;
    private AppCompatEditText methodNameEdit;
    private AppCompatEditText descriptorEdit;
    private AppCompatEditText registerCountEdit;
    private ClassDefItem classDef;
    private EncodedMethod method;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.method_item_editor);
        TextWatcher watch = new TextWatcher() {
            public void beforeTextChanged(CharSequence c, int start, int count,
                                          int after) {
            }

            public void onTextChanged(CharSequence c, int start, int count,
                                      int after) {
            }

            public void afterTextChanged(Editable edit) {
                if (!isChanged)
                    isChanged = true;
            }
        };
        accessFlagsEdit = findViewById(R.id.access_flags_edit);
        accessFlagsEdit.addTextChangedListener(watch);
        methodNameEdit = findViewById(R.id.method_name_edit);
        methodNameEdit.addTextChangedListener(watch);
        descriptorEdit = findViewById(R.id.method_descriptor_edit);
        descriptorEdit.addTextChangedListener(watch);
        registerCountEdit = findViewById(R.id.register_count_edit);
        registerCountEdit.addTextChangedListener(watch);
        init();
    }

    @SuppressLint("SetTextI18n")
    public void init() {
        classDef = ClassListActivity.curClassDef;
        accessFlagsEdit.setText("");
        methodNameEdit.setText("newMethod");
        descriptorEdit.setText("()V");
        registerCountEdit.setText("1");
        isChanged = true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (isChanged) {
                MainActivity.prompt(this, getString(R.string.prompt),
                        getString(R.string.is_save),
                        (dailog, which) -> {
                            if (which == AlertDialog.BUTTON_POSITIVE) {
                                if (save(ClassListActivity.dexFile)) {
                                    setResult(ActResConstant.add_method,
                                            getIntent());
                                    finish();
                                }
                            } else if (which == AlertDialog.BUTTON_NEGATIVE) {
                                ClassListActivity.isChanged = false;
                                finish();
                            }
                        });
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    private boolean save(DexFile dexFile) {
        String[] str = null;
        // access Flags
        int accessFlags = 0;
        try {
            String ac = accessFlagsEdit.getText().toString();
            if (ac != null && !ac.equals("")) {
                str = pattern.split(accessFlagsEdit.getText().toString());
                if (str != null) {
                    for (String s : str) {
                        AccessFlags accessFlag = AccessFlags.getAccessFlag(s);
                        accessFlags |= accessFlag.getValue();
                    }
                }
            }
        } catch (Exception e) {
            MainActivity.showMessage(this, "", "Access Flag Error ");
            return false;
        }
        try {
            str = pParams.split(descriptorEdit.getText().toString());
            if (str[str.length - 1].equals(""))
                throw new Exception("No Return Type Exception");
            TypeListItem typeList = Parser.buildTypeList(dexFile,
                    str[str.length - 2]);
            TypeIdItem returnType = TypeIdItem.internTypeIdItem(dexFile,
                    str[str.length - 1]);
            MethodIdItem method = MethodIdItem.internMethodIdItem(dexFile,
                    classDef.getClassType(), ProtoIdItem.internProtoIdItem(
                            dexFile, returnType, typeList), StringIdItem
                            .internStringIdItem(dexFile, methodNameEdit
                                    .getText().toString()));
            CodeItem codeItem = null;
            if ((accessFlags | AccessFlags.ABSTRACT.getValue()) == 0
                    || (accessFlags | AccessFlags.NATIVE.getValue()) == 0) {
                int regCount = Integer.parseInt(registerCountEdit.getText()
                        .toString().trim());
                List<Instruction> instructions = new ArrayList<Instruction>();
                instructions.add(new Instruction10x(Opcode.RETURN_VOID));
                codeItem = CodeItem.internCodeItem(dexFile, regCount, 1, 0,
                        null,/* debuginfo */
                        instructions, null, null);
            }
            classDef.getClassData().addMethod(
                    new EncodedMethod(method, accessFlags, codeItem));
            ClassListActivity.isChanged = true;
            isChanged = false;
        } catch (Exception e) {
            MainActivity.showMessage(this, "", "Method Error");
            return false;
        }
        return true;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        clearAll();
    }

    private void clearAll() {
        accessFlagsEdit = null;
        methodNameEdit = null;
        descriptorEdit = null;
        registerCountEdit = null;
        classDef = null;
        method = null;
        System.gc();
    }
}
