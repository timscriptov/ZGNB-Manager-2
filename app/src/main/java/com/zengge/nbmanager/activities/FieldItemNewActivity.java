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

import org.jf.dexlib.ClassDataItem.EncodedField;
import org.jf.dexlib.ClassDefItem;
import org.jf.dexlib.DexFile;
import org.jf.dexlib.FieldIdItem;
import org.jf.dexlib.StringIdItem;
import org.jf.dexlib.TypeIdItem;
import org.jf.dexlib.Util.AccessFlags;

import java.util.regex.Pattern;

public class FieldItemNewActivity extends AppCompatActivity {
    public static final Pattern pattern = Pattern.compile("\\s");
    private boolean isChanged;
    private AppCompatEditText accessFlagsEdit;
    private AppCompatEditText fieldNameEdit;
    private AppCompatEditText descriptorEdit;
    private ClassDefItem classDef;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.field_item_editor);
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
        fieldNameEdit = findViewById(R.id.field_name_edit);
        fieldNameEdit.addTextChangedListener(watch);
        descriptorEdit = findViewById(R.id.field_descriptor_edit);
        descriptorEdit.addTextChangedListener(watch);
        init();
    }

    @SuppressLint("SetTextI18n")
    private void init() {
        classDef = ClassListActivity.curClassDef;
        accessFlagsEdit.setText("");
        fieldNameEdit.setText("newField");
        descriptorEdit.setText("Ljava/lang/String;");
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
                                    setResult(ActResConstant.add_field,
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
        String[] str;
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
            FieldIdItem field = FieldIdItem.internFieldIdItem(dexFile, classDef
                    .getClassType(), TypeIdItem.internTypeIdItem(dexFile,
                    descriptorEdit.getText().toString()), StringIdItem
                    .internStringIdItem(dexFile, fieldNameEdit.getText()
                            .toString()));
            classDef.getClassData().addField(
                    new EncodedField(field, accessFlags));
            ClassListActivity.isChanged = true;
            isChanged = false;
        } catch (Exception e) {
            MainActivity.showMessage(this, "", "Field Name or Descriptor Error");
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
        classDef = null;
        accessFlagsEdit = null;
        fieldNameEdit = null;
        descriptorEdit = null;
        System.gc();
    }
}
