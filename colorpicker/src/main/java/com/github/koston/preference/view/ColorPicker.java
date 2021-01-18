/*
 * Copyright 2012 Lars Werkman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.github.koston.preference.view;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.SweepGradient;
import android.os.Bundle;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.github.koston.preference.R;

import org.jetbrains.annotations.NotNull;

@SuppressWarnings("unused")
public class ColorPicker extends View {

    public static final int SOURCE_OUTSIDE = -2;
    public static final int TYPE_OPACITY = -1;
    public static final int TYPE_PICKER = 0;
    public static final int TYPE_SATURATION = 1;
    public static final int TYPE_VALUE = 2;

    private static final String TAG = "ColorPicker";
    private static final String STATE_PARENT = "parent";
    private static final String STATE_ANGLE = "angle";
    private static final String STATE_ALPHA = "alpha";
    private static final String STATE_HSV = "hsv";
    private static final String STATE_OLD_COLOR = "color";
    private static final String STATE_SHOW_OLD_COLOR = "showColor";

    private static final int[] COLORS =
            new int[]{
                    0xFFFF0000, 0xFFFF00FF, 0xFF0000FF, 0xFF00FFFF, 0xFF00FF00, 0xFFFFFF00, 0xFFFF0000
            };

    private Paint mColorWheelPaint;
    private Paint mPointerHaloPaint;
    private Paint mPointerColor;

    private int mColorWheelThickness;
    private int mColorWheelRadius;
    private int mColorCenterRadius;
    private int mColorCenterHaloRadius;
    private int mColorPointerRadius;
    private int mColorPointerHaloRadius;
    private int mColorPointerHaloColor;

    private int mPreferredColorWheelRadius;
    private int mPreferredColorCenterRadius;
    private int mPreferredColorCenterHaloRadius;

    private RectF mColorWheelRectangle = new RectF();
    private RectF mCenterRectangle = new RectF();

    private boolean mUserIsMovingPointer = false;
    private boolean mShowCenterOldColor;
    private boolean mShowCenter;
    private boolean mTouchAnywhereOnColorWheelEnabled = true;

    private int mCenterOldColor;
    private int mCenterNewColor;
    private int mAlpha;

    private float mTranslationOffset;
    private float mSlopX;
    private float mSlopY;
    private float mAngle;

    private Paint mCenterOldPaint;
    private Paint mCenterNewPaint;
    private Paint mCenterHaloPaint;

    private float[] mHSV = new float[3];

    private OpacityBar mOpacityBar = null;
    private SaturationValueBar mSaturationBar = null;
    private SaturationValueBar mValueBar = null;

    private OnColorChangedListener onColorChangedListener;
    private OnColorSelectedListener onColorSelectedListener;

    private int oldChangedListenerColor;
    private int oldSelectedListenerColor;

    public ColorPicker(Context context) {
        super(context);
        init(null, 0);
    }

    public ColorPicker(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    public ColorPicker(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(attrs, defStyle);
    }

    public OnColorChangedListener getOnColorChangedListener() {
        return this.onColorChangedListener;
    }

    public void setOnColorChangedListener(OnColorChangedListener listener) {
        this.onColorChangedListener = listener;
    }

    public OnColorSelectedListener getOnColorSelectedListener() {
        return this.onColorSelectedListener;
    }

    public void setOnColorSelectedListener(OnColorSelectedListener listener) {
        this.onColorSelectedListener = listener;
    }

    private void init(AttributeSet attrs, int defStyle) {
        final TypedArray a =
                getContext().obtainStyledAttributes(attrs, R.styleable.ColorPicker, defStyle, 0);
        final Resources r = getContext().getResources();

        mColorWheelThickness =
                a.getDimensionPixelSize(
                        R.styleable.ColorPicker_wheelThickness,
                        r.getDimensionPixelSize(R.dimen.defaultWheelThickness));
        mColorWheelRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPicker_wheelRadius,
                        r.getDimensionPixelSize(R.dimen.defaultWheelRadius));
        mPreferredColorWheelRadius = mColorWheelRadius;
        mColorCenterRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPicker_centerCircleRadius,
                        r.getDimensionPixelSize(R.dimen.defaultCenterRadius));
        mPreferredColorCenterRadius = mColorCenterRadius;
        mColorCenterHaloRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPicker_centerCircleHaloRadius,
                        r.getDimensionPixelSize(R.dimen.defaultCenterHaloRadius));
        mPreferredColorCenterHaloRadius = mColorCenterHaloRadius;
        mColorPointerRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPicker_pointerRadius,
                        r.getDimensionPixelSize(R.dimen.defaultPointerRadius));
        mColorPointerHaloRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPicker_pointerHaloRadius,
                        r.getDimensionPixelSize(R.dimen.defaultPointerHaloRadius));
        mColorPointerHaloColor =
                a.getColor(
                        R.styleable.ColorPicker_pointerHaloColor, r.getColor(R.color.defaultPointerHaloColor));

        a.recycle();

        mAngle = (float) (-Math.PI / 2);

        Shader s = new SweepGradient(0, 0, COLORS, null);

        mColorWheelPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mColorWheelPaint.setShader(s);
        mColorWheelPaint.setStyle(Paint.Style.STROKE);
        mColorWheelPaint.setStrokeWidth(mColorWheelThickness);

        mPointerHaloPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mPointerHaloPaint.setColor(mColorPointerHaloColor);

        mPointerColor = new Paint(Paint.ANTI_ALIAS_FLAG);
        mPointerColor.setColor(setHueFromAngleRGB(mAngle, COLORS[0]));

        mCenterNewPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mCenterNewPaint.setColor(setHueFromAngleRGB(mAngle, COLORS[0]));
        mCenterNewPaint.setStyle(Paint.Style.FILL);

        mCenterOldPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mCenterOldPaint.setColor(setHueFromAngleRGB(mAngle, COLORS[0]));
        mCenterOldPaint.setStyle(Paint.Style.FILL);

        mCenterHaloPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mCenterHaloPaint.setColor(Color.BLACK);
        mCenterHaloPaint.setAlpha(0x00);

        mCenterNewColor = setHueFromAngleRGB(mAngle, COLORS[0]);
        mCenterOldColor = setHueFromAngleRGB(mAngle, COLORS[0]);
        mShowCenterOldColor = true;
    }

    @Override
    protected void onDraw(@NotNull Canvas canvas) {
        // All of our positions are using our internal coordinate system.
        // Instead of translating
        // them we let Canvas do the work for us.
        canvas.translate(mTranslationOffset, mTranslationOffset);

        // Draw the color wheel.
        canvas.drawOval(mColorWheelRectangle, mColorWheelPaint);

        float[] pointerPosition = calculatePointerPosition(mAngle);

        // Draw the pointer's "halo"
        canvas.drawCircle(
                pointerPosition[0], pointerPosition[1], mColorPointerHaloRadius, mPointerHaloPaint);

        // Draw the pointer (the currently selected color) slightly smaller on
        // top.
        canvas.drawCircle(pointerPosition[0], pointerPosition[1], mColorPointerRadius, mPointerColor);

        if (mShowCenter) {

            // Draw the halo of the center colors.
            canvas.drawCircle(0, 0, mColorCenterHaloRadius, mCenterHaloPaint);

            if (mShowCenterOldColor) {
                // Draw the old selected color in the center.
                canvas.drawArc(mCenterRectangle, 90, 180, true, mCenterOldPaint);

                // Draw the new selected color in the center.
                canvas.drawArc(mCenterRectangle, 270, 180, true, mCenterNewPaint);
            } else {
                // Draw the new selected color in the center.
                canvas.drawArc(mCenterRectangle, 0, 360, true, mCenterNewPaint);
            }
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int intrinsicSize = 2 * (mPreferredColorWheelRadius + mColorPointerHaloRadius);

        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        int widthSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);

        int width;
        int height;

        if (widthMode == MeasureSpec.EXACTLY) {
            width = widthSize;
        } else if (widthMode == MeasureSpec.AT_MOST) {
            width = Math.min(intrinsicSize, widthSize);
        } else {
            width = intrinsicSize;
        }

        if (heightMode == MeasureSpec.EXACTLY) {
            height = heightSize;
        } else if (heightMode == MeasureSpec.AT_MOST) {
            height = Math.min(intrinsicSize, heightSize);
        } else {
            height = intrinsicSize;
        }

        int min = Math.min(width, height);
        setMeasuredDimension(min, min);
        mTranslationOffset = min * 0.5f;

        // fill the rectangle instances.
        mColorWheelRadius = min / 2 - mColorWheelThickness - mColorPointerHaloRadius;
        mColorWheelRectangle.set(
                -mColorWheelRadius, -mColorWheelRadius, mColorWheelRadius, mColorWheelRadius);

        mColorCenterRadius =
                (int)
                        ((float) mPreferredColorCenterRadius
                                * ((float) mColorWheelRadius / (float) mPreferredColorWheelRadius));
        mColorCenterHaloRadius =
                (int)
                        ((float) mPreferredColorCenterHaloRadius
                                * ((float) mColorWheelRadius / (float) mPreferredColorWheelRadius));
        mCenterRectangle.set(
                -mColorCenterRadius, -mColorCenterRadius, mColorCenterRadius, mColorCenterRadius);
    }

    private int ave(int s, int d, float p) {
        return s + Math.round(p * (d - s));
    }

    private int calculateHueColorHelper(int color, int colorForHue) {
        float[] hsv = {0, 0, 0};
        float[] hue = {0, 0, 0};
        Color.colorToHSV(color, hsv);
        Color.colorToHSV(colorForHue, hue);
        return Color.HSVToColor(Color.alpha(color), new float[]{hue[0], hsv[1], hsv[2]});
    }

    private int setHueFromAngleRGB(float angle, int prevColor) {
        float unit = (float) (angle / (2 * Math.PI));
        if (unit < 0) {
            unit += 1;
        }

        if (unit <= 0) {
            return calculateHueColorHelper(prevColor, COLORS[0]);
        }
        if (unit >= 1) {
            return calculateHueColorHelper(prevColor, COLORS[COLORS.length - 1]);
        }

        float p = unit * (COLORS.length - 1);
        int i = (int) p;
        p -= i;

        int c0 = COLORS[i];
        int c1 = COLORS[i + 1];
        int a = ave(Color.alpha(c0), Color.alpha(c1), p);
        int r = ave(Color.red(c0), Color.red(c1), p);
        int g = ave(Color.green(c0), Color.green(c1), p);
        int b = ave(Color.blue(c0), Color.blue(c1), p);

        return calculateHueColorHelper(prevColor, Color.argb(a, r, g, b));
    }

    private void setHueFromAngleHSV(float angle, float[] color) {
        float degrees = -(float) Math.toDegrees((double) angle);
        float hue = degrees;
        if (degrees < 0) {
            hue = degrees + 360;
        }
        color[0] = hue;
    }

    public int getColor() {
        return mCenterNewColor;
    }

    public void setColor(int color) {
        float[] hsv = new float[3];
        Color.colorToHSV(color, hsv);
        setColor(Color.alpha(color), hsv, SOURCE_OUTSIDE);
    }

    public void setColor(int alpha, float @NotNull [] color, int source) {

        // A-HSV handling
        mAlpha = alpha;
        mHSV = color;
        mAngle = (float) Math.toRadians(-color[0]);

        // Update Visuals
        int rgbCol = Color.HSVToColor(alpha, color);
        mPointerColor.setColor(rgbCol);
        setNewCenterColor(rgbCol);

        // Communicate
        if (mOpacityBar != null && source != TYPE_OPACITY) {
            mOpacityBar.initializeColor(alpha, color);
        }

        if (mSaturationBar != null && source != mSaturationBar.getType()) {
            mSaturationBar.initializeColor(alpha, color);
        }

        if (mValueBar != null && source != mValueBar.getType()) {
            mValueBar.initializeColor(alpha, color);
        }
    }

    public void initializeColor(int argb, int source) {
        float[] hsv = new float[3];
        Color.colorToHSV(argb, hsv);
        setColor(Color.alpha(argb), hsv, source);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(@NotNull MotionEvent event) {
        getParent().requestDisallowInterceptTouchEvent(true);

        // Convert coordinates to our internal coordinate system
        float x = event.getX() - mTranslationOffset;
        float y = event.getY() - mTranslationOffset;

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                // Check whether the user pressed on the pointer.
                float[] pointerPosition = calculatePointerPosition(mAngle);
                if (x >= (pointerPosition[0] - mColorPointerHaloRadius)
                        && x <= (pointerPosition[0] + mColorPointerHaloRadius)
                        && y >= (pointerPosition[1] - mColorPointerHaloRadius)
                        && y <= (pointerPosition[1] + mColorPointerHaloRadius)) {
                    mSlopX = x - pointerPosition[0];
                    mSlopY = y - pointerPosition[1];
                    mUserIsMovingPointer = true;
                    invalidate();
                }
                // Check whether the user pressed on the center.
                else if (x >= -mColorCenterRadius
                        && x <= mColorCenterRadius
                        && y >= -mColorCenterRadius
                        && y <= mColorCenterRadius
                        && mShowCenterOldColor) {
                    mCenterHaloPaint.setAlpha(0x50);
                    setColor(mAlpha, getOldCenterColorHSV(), TYPE_PICKER);
                    invalidate();
                }
                // Check whether the user pressed anywhere on the wheel.
                else {
                    double sqrt = Math.sqrt(x * x + y * y);
                    if (sqrt <= mColorWheelRadius + mColorPointerHaloRadius
                            && sqrt >= mColorWheelRadius - mColorPointerHaloRadius
                            && mTouchAnywhereOnColorWheelEnabled) {
                        mUserIsMovingPointer = true;
                        invalidate();
                    }
                    // If user did not press pointer or center, report event not handled
                    else {
                        getParent().requestDisallowInterceptTouchEvent(false);
                        return false;
                    }
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (mUserIsMovingPointer) {
                    mAngle = (float) Math.atan2(y - mSlopY, x - mSlopX);
                    setHueFromAngleHSV(mAngle, mHSV);
                    setNewCenterColor(mCenterNewColor = Color.HSVToColor(mAlpha, mHSV));

                    setColor(mAlpha, mHSV, TYPE_PICKER);

                    invalidate();
                }
                // If user did not press pointer or center, report event not handled
                else {
                    getParent().requestDisallowInterceptTouchEvent(false);
                    return false;
                }
                break;
            case MotionEvent.ACTION_UP:
                mUserIsMovingPointer = false;
                mCenterHaloPaint.setAlpha(0x00);

                if (onColorSelectedListener != null && mCenterNewColor != oldSelectedListenerColor) {
                    onColorSelectedListener.onColorSelected(mCenterNewColor);
                    oldSelectedListenerColor = mCenterNewColor;
                }

                invalidate();
                break;
            case MotionEvent.ACTION_CANCEL:
                if (onColorSelectedListener != null && mCenterNewColor != oldSelectedListenerColor) {
                    onColorSelectedListener.onColorSelected(mCenterNewColor);
                    oldSelectedListenerColor = mCenterNewColor;
                }
                break;
        }
        return true;
    }

    private float @NotNull [] calculatePointerPosition(float angle) {
        float x = (float) (mColorWheelRadius * Math.cos(angle));
        float y = (float) (mColorWheelRadius * Math.sin(angle));

        return new float[]{x, y};
    }

    public void addOpacityBar(OpacityBar bar) {
        mOpacityBar = bar;
        mOpacityBar.setColorPicker(this);
    }

    public void addSaturationBar(SaturationValueBar bar) {
        mSaturationBar = bar;
        mSaturationBar.setColorPicker(this);
    }

    public void addValueBar(SaturationValueBar bar) {
        mValueBar = bar;
        mValueBar.setColorPicker(this);
    }

    public void setNewCenterColor(int color) {
        mCenterNewColor = color;
        mCenterNewPaint.setColor(color);
        if (mCenterOldColor == 0) {
            mCenterOldColor = color;
            mCenterOldPaint.setColor(color);
        }
        if (onColorChangedListener != null && color != oldChangedListenerColor) {
            onColorChangedListener.onColorChanged(color);
            oldChangedListenerColor = color;
        }
        invalidate();
    }

    public float[] getOldCenterColorHSV() {
        float[] hsv = new float[3];
        Color.colorToHSV(getOldCenterColor(), hsv);
        return hsv;
    }

    public int getOldCenterColor() {
        return mCenterOldColor;
    }

    public void setOldCenterColor(int color) {
        mCenterOldColor = color;
        mCenterOldPaint.setColor(color);
        invalidate();
    }

    public boolean getShowOldCenterColor() {
        return mShowCenterOldColor;
    }

    public void setShowOldCenterColor(boolean show) {
        mShowCenterOldColor = show;
        invalidate();
    }

    public boolean getShowCenter() {
        return mShowCenter;
    }

    public void setShowCenter(boolean show) {
        mShowCenter = show;
        invalidate();
    }

    public boolean hasOpacityBar() {
        return mOpacityBar != null;
    }

    public boolean hasValueBar() {
        return mValueBar != null;
    }

    public boolean hasSaturationBar() {
        return mSaturationBar != null;
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();

        Bundle state = new Bundle();
        state.putParcelable(STATE_PARENT, superState);
        state.putFloat(STATE_ANGLE, mAngle);
        state.putInt(STATE_ALPHA, mAlpha);
        state.putFloatArray(STATE_HSV, mHSV);
        state.putInt(STATE_OLD_COLOR, mCenterOldColor);
        state.putBoolean(STATE_SHOW_OLD_COLOR, mShowCenterOldColor);

        return state;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        Bundle savedState = (Bundle) state;

        Parcelable superState = savedState.getParcelable(STATE_PARENT);
        super.onRestoreInstanceState(superState);

        mAngle = savedState.getFloat(STATE_ANGLE);
        setOldCenterColor(savedState.getInt(STATE_OLD_COLOR));
        mShowCenterOldColor = savedState.getBoolean(STATE_SHOW_OLD_COLOR);
        mAlpha = savedState.getInt(STATE_ALPHA);
        mHSV = savedState.getFloatArray(STATE_HSV);
        int currentColor = Color.HSVToColor(mHSV);
        mPointerColor.setColor(currentColor);
        setNewCenterColor(currentColor);
    }

    public void setTouchAnywhereOnColorWheelEnabled(boolean TouchAnywhereOnColorWheelEnabled) {
        mTouchAnywhereOnColorWheelEnabled = TouchAnywhereOnColorWheelEnabled;
    }

    public boolean getTouchAnywhereOnColorWheel() {
        return mTouchAnywhereOnColorWheelEnabled;
    }

    private void logHSV(String source, float @NotNull [] mHSVColor) {
        Log.d(TAG, source + ": " + mHSVColor[0] + "/" + mHSVColor[1] + "/" + mHSVColor[2]);
    }

    public void setColorWheelThickness(int colorWheelThickness) {
        this.mColorWheelThickness = colorWheelThickness;
    }

    public void setColorWheelRadius(int colorWheelRadius) {
        this.mColorWheelRadius = colorWheelRadius;
    }

    public void setColorCenterRadius(int colorCenterRadius) {
        this.mColorCenterRadius = colorCenterRadius;
    }

    public void setColorCenterHaloRadius(int colorCenterHaloRadius) {
        this.mColorCenterHaloRadius = colorCenterHaloRadius;
    }

    public void setColorPointerRadius(int colorPointerRadius) {
        this.mColorPointerRadius = colorPointerRadius;
    }

    public void setColorPointerHaloRadius(int colorPointerHaloRadius) {
        this.mColorPointerHaloRadius = colorPointerHaloRadius;
    }

    public void setColorPointerHaloColor(int colorPointerHaloColor) {
        this.mColorPointerHaloColor = colorPointerHaloColor;
        mPointerHaloPaint.setColor(mColorPointerHaloColor);
    }

    public interface OnColorChangedListener {

        void onColorChanged(int color);
    }

    public interface OnColorSelectedListener {

        void onColorSelected(int color);
    }
}
