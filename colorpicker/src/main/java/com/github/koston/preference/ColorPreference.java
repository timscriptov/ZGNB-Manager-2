package com.github.koston.preference;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.core.content.res.ResourcesCompat;
import androidx.preference.DialogPreference;
import androidx.preference.PreferenceViewHolder;

import org.jetbrains.annotations.NotNull;

@SuppressWarnings("unused")
public class ColorPreference extends DialogPreference {

    private int mColor;
    private Drawable mDrawable;
    private boolean asIndicator;
    private AppCompatImageView ivIndicator;

    private int pointersHaloColor;

    private int colorWheelThickness;
    private int colorWheelRadius;
    private int colorCenterRadius;
    private int colorCenterHaloRadius;
    private int colorPointerRadius;
    private int colorPointerHaloRadius;

    private int barThickness;
    private int barLength;
    private int barPointerRadius;
    private int barPointerHaloRadius;

    public ColorPreference(Context context) {
        super(context);
        init(context, null);
    }

    public ColorPreference(Context context, AttributeSet attrs) {
        super(context, attrs, R.attr.dialogPreferenceStyle);
        init(context, attrs);
    }

    public ColorPreference(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    private void init(@NotNull Context context, AttributeSet attrs) {
        Resources b = context.getResources();
        mDrawable = ResourcesCompat.getDrawable(b, R.drawable.circle, context.getTheme());

        if (attrs != null) {
            TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ColorPreference);

            asIndicator = a.getBoolean(R.styleable.ColorPreference_indicatorColorPreview, true);

            colorWheelThickness =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_hueWheelThickness,
                            b.getDimensionPixelSize(R.dimen.defaultWheelThickness));
            colorWheelRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_hueWheelRadius,
                            b.getDimensionPixelSize(R.dimen.defaultWheelRadius));
            colorCenterRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_hueCenterCircleRadius,
                            b.getDimensionPixelSize(R.dimen.defaultCenterRadius));
            colorCenterHaloRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_hueCenterCircleHaloRadius,
                            b.getDimensionPixelSize(R.dimen.defaultCenterHaloRadius));
            colorPointerRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_huePointerRadius,
                            b.getDimensionPixelSize(R.dimen.defaultPointerRadius));
            colorPointerHaloRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_huePointerHaloRadius,
                            b.getDimensionPixelSize(R.dimen.defaultPointerHaloRadius));

            barThickness =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_barsThickness,
                            b.getDimensionPixelSize(R.dimen.defaultBarThickness));
            barLength =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_barsLength,
                            b.getDimensionPixelSize(R.dimen.defaultBarLength));
            barPointerRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_barsPointerRadius,
                            b.getDimensionPixelSize(R.dimen.defaultBarPointerRadius));
            barPointerHaloRadius =
                    a.getDimensionPixelSize(
                            R.styleable.ColorPreference_barsPointerHaloRadius,
                            b.getDimensionPixelSize(R.dimen.defaultBarPointerHaloRadius));
            pointersHaloColor =
                    a.getColor(
                            R.styleable.ColorPreference_pointersHaloColor,
                            b.getColor(R.color.defaultPointerHaloColor));

            a.recycle();
        }

        setWidgetLayoutResource(R.layout.preference_indicator);
    }

    @Override
    protected Object onGetDefaultValue(@NotNull TypedArray a, int index) {
        return a.getInt(index, 0);
    }

    @Override
    public void onBindViewHolder(@NotNull PreferenceViewHolder holder) {
        ivIndicator = (AppCompatImageView) holder.findViewById(R.id.colorIndicator);
        super.onBindViewHolder(holder);
        setColor(mColor);
    }

    private void setIndicatorColor() {
        if (asIndicator && ivIndicator != null) {
            mDrawable.setColorFilter(new PorterDuffColorFilter(mColor, PorterDuff.Mode.SRC_IN));
            ivIndicator.setImageDrawable(mDrawable);
        }
    }

    @Override
    @SuppressWarnings("deprecation")
    protected void onSetInitialValue(boolean restorePersistedValue, Object defaultValue) {
        setColor(restorePersistedValue ? getPersistedInt(mColor) : (int) defaultValue);
    }

    @Override
    public int getDialogLayoutResource() {
        return R.layout.dialog_color_picker;
    }

    @Override
    protected void onClick() {
        getPreferenceManager().showDialog(this);
    }

    public int getColor() {
        return mColor;
    }

    public void setColor(int color) {
        mColor = color;
        setIndicatorColor();
        persistInt(mColor);
    }

    public int getColorWheelThickness() {
        return colorWheelThickness;
    }

    public int getColorWheelRadius() {
        return colorWheelRadius;
    }

    public int getColorCenterRadius() {
        return colorCenterRadius;
    }

    public int getColorCenterHaloRadius() {
        return colorCenterHaloRadius;
    }

    public int getColorPointerRadius() {
        return colorPointerRadius;
    }

    public int getColorPointerHaloRadius() {
        return colorPointerHaloRadius;
    }

    public int getBarThickness() {
        return barThickness;
    }

    public int getBarLength() {
        return barLength;
    }

    public int getBarPointerRadius() {
        return barPointerRadius;
    }

    public int getBarPointerHaloRadius() {
        return barPointerHaloRadius;
    }

    public int getPointersHaloColor() {
        return pointersHaloColor;
    }
}
