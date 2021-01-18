package com.github.koston.preference;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.inputmethod.EditorInfo;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.preference.PreferenceDialogFragmentCompat;

import com.github.koston.preference.view.ColorPicker;
import com.github.koston.preference.view.OpacityBar;
import com.github.koston.preference.view.SaturationValueBar;

import org.jetbrains.annotations.NotNull;

@SuppressWarnings("unused")
public class ColorPickerDialog extends PreferenceDialogFragmentCompat
        implements ColorPicker.OnColorChangedListener, TextWatcher {

    private ColorPicker picker;
    private AppCompatEditText hex;

    private ColorPreference mPreference;

    private boolean hexChanging;

    static @NotNull ColorPickerDialog newInstance(String key) {
        ColorPickerDialog dialog = new ColorPickerDialog();
        Bundle args = new Bundle(1);
        args.putString(ARG_KEY, key);
        dialog.setArguments(args);
        return dialog;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null) {
            hexChanging = savedInstanceState.getBoolean("hexChanging");
        }
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean("hexChanging", hexChanging);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        mPreference = (ColorPreference) getPreference();
        int color = mPreference.getColor();

        picker = view.findViewById(R.id.hue);
        hex = view.findViewById(R.id.hex);
        SaturationValueBar saturationBar = view.findViewById(R.id.saturationBar);
        OpacityBar opacityBar = view.findViewById(R.id.opacityBar);
        SaturationValueBar valueBar = view.findViewById(R.id.valueBar);

        int barThickness = mPreference.getBarThickness();
        int barLength = mPreference.getBarLength();
        int barPointerRadius = mPreference.getBarPointerRadius();
        int barPointerHaloRadius = mPreference.getBarPointerHaloRadius();
        int pointersHaloColor = mPreference.getPointersHaloColor();

        picker.setColorWheelRadius(mPreference.getColorWheelRadius());
        picker.setColorWheelThickness(mPreference.getColorWheelThickness());
        picker.setColorCenterRadius(mPreference.getColorCenterRadius());
        picker.setColorCenterHaloRadius(mPreference.getColorCenterHaloRadius());
        picker.setColorPointerRadius(mPreference.getColorPointerRadius());
        picker.setColorPointerHaloRadius(mPreference.getBarPointerHaloRadius());
        picker.setColorPointerHaloColor(pointersHaloColor);

        saturationBar.setBarThickness(barThickness);
        saturationBar.setBarLength(barLength);
        saturationBar.setBarPointerRadius(barPointerRadius);
        saturationBar.setBarPointerHaloRadius(barPointerHaloRadius);
        saturationBar.setBarPointerHaloColor(pointersHaloColor);

        opacityBar.setBarThickness(barThickness);
        opacityBar.setBarLength(barLength);
        opacityBar.setBarPointerRadius(barPointerRadius);
        opacityBar.setBarPointerHaloRadius(barPointerHaloRadius);
        opacityBar.setBarPointerHaloColor(pointersHaloColor);

        valueBar.setBarThickness(barThickness);
        valueBar.setBarLength(barLength);
        valueBar.setBarPointerRadius(barPointerRadius);
        valueBar.setBarPointerHaloRadius(barPointerHaloRadius);
        valueBar.setBarPointerHaloColor(pointersHaloColor);

        picker.addSaturationBar(saturationBar);
        picker.addValueBar(valueBar);
        picker.addOpacityBar(opacityBar);
        picker.setShowCenter(true);
        picker.setOldCenterColor(color);
        picker.setOnColorChangedListener(this);
        picker.initializeColor(color, ColorPicker.SOURCE_OUTSIDE);

        hex.setText(Integer.toHexString(color).toUpperCase());
        hex.addTextChangedListener(this);
        hexChanging = true;
    }

    @Override
    public void onDialogClosed(boolean positiveResult) {
        if (positiveResult) {
            int color = picker.getColor();
            if (mPreference.callChangeListener(color)) {
                mPreference.setColor(color);
            }
        }
    }

    @Override
    public void onColorChanged(int newColor) {
        if (hexChanging) {
            hexChanging = false;
            hex.setText(Integer.toHexString(newColor).toUpperCase());
            hexChanging = true;
        }
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
    }

    @Override
    public void afterTextChanged(Editable s) {
        if (hexChanging) {
            if (s.length() == 8) {
                picker.setColor((int) Long.parseLong(s.toString(), 16));
                hex.onEditorAction(EditorInfo.IME_ACTION_DONE);
                hex.clearFocus();
            }
        }
    }
}
