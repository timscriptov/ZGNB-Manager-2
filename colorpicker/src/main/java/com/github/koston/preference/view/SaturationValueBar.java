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
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.Bundle;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.github.koston.preference.R;

import org.jetbrains.annotations.NotNull;

@SuppressWarnings("unused")
public class SaturationValueBar extends View {

    private static final String TAG = "SaturationValueBar";

    private static final String STATE_PARENT = "parent";
    private static final String STATE_COLOR = "color";
    private static final String STATE_ALPHA = "alpha";

    private int mBarType;
    private int mBarThickness;
    private int mBarLength;
    private int mBarPointerRadius;
    private int mBarPointerHaloRadius;
    private int mBarPointerHaloColor;
    private int mBarPointerPosition;
    private boolean mBarIsHorizontal;

    private int mPreferredBarLength;

    private Paint mBarPaint;
    private Paint mBarPointerPaint;
    private Paint mBarPointerHaloPaint;
    private RectF mBarRect = new RectF();

    private Shader shader;

    private boolean mIsMovingPointer;

    private int mAlpha;
    private float[] mHSVColor = new float[3];

    private float mPosToSVFactor;
    private float mSVToPosFactor;

    private OnOmniChangedListener onOmniChangedListener;
    private int oldChangedListenerColor;

    private ColorPicker mPicker = null;

    public SaturationValueBar(Context context) {
        super(context);
        init(null, 0);
    }

    public SaturationValueBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    public SaturationValueBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(attrs, defStyle);
    }

    private void init(AttributeSet attrs, int defStyle) {
        @SuppressLint("CustomViewStyleable") final TypedArray a =
                getContext().obtainStyledAttributes(attrs, R.styleable.ColorPickerBars, defStyle, 0);
        final Resources r = getContext().getResources();

        int type = a.getInt(R.styleable.ColorPickerBars_barType, ColorPicker.SOURCE_OUTSIDE);
        if (type == ColorPicker.TYPE_SATURATION || type == ColorPicker.TYPE_VALUE) {
            mBarType = type;
        } else {
            Log.w(TAG, "assign 'bar_type' in XML Layout, SaturationValue otherwise inoperable");
        }

        mBarThickness =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barThickness,
                        r.getDimensionPixelSize(R.dimen.defaultBarThickness));
        mBarLength =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barLength,
                        r.getDimensionPixelSize(R.dimen.defaultBarLength));
        mPreferredBarLength = mBarLength;
        mBarPointerRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barPointerRadius,
                        r.getDimensionPixelSize(R.dimen.defaultBarPointerRadius));
        mBarPointerHaloRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barPointerHaloRadius,
                        r.getDimensionPixelSize(R.dimen.defaultBarPointerHaloRadius));
        mBarPointerHaloColor =
                a.getColor(
                        R.styleable.ColorPickerBars_barPointerHaloColor,
                        r.getColor(R.color.defaultPointerHaloColor));
        mBarIsHorizontal = a.getBoolean(R.styleable.ColorPickerBars_barOrientationHorizontal, true);

        a.recycle();

        mBarPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBarPaint.setShader(shader);

        mBarPointerPosition = mBarLength + mBarPointerHaloRadius;

        mBarPointerHaloPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBarPointerHaloPaint.setColor(mBarPointerHaloColor);

        mBarPointerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBarPointerPaint.setColor(0xff81ff00);

        mPosToSVFactor = 1 / ((float) mBarLength);
        mSVToPosFactor = ((float) mBarLength) / 1;
    }

    public int getType() {
        return mBarType;
    }

    public OnOmniChangedListener getOnOmniChangedListener() {
        return this.onOmniChangedListener;
    }

    public void setOnOmniChangedListener(OnOmniChangedListener listener) {
        this.onOmniChangedListener = listener;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        getParent().requestDisallowInterceptTouchEvent(true);

        // Convert coordinates to our internal coordinate system
        float dimen;
        if (mBarIsHorizontal) {
            dimen = event.getX();
        } else {
            dimen = event.getY();
        }

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                mIsMovingPointer = true;
                // Check whether the user pressed on (or near) the pointer
                if (dimen >= (mBarPointerHaloRadius) && dimen <= (mBarPointerHaloRadius + mBarLength)) {
                    mBarPointerPosition = Math.round(dimen);
                    setSVFromCoordinates(dimen);
                    invalidate();
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (mIsMovingPointer) {
                    // Move the the pointer on the bar.
                    // Touch Event happens on the bar inside the end points
                    if (dimen >= mBarPointerHaloRadius && dimen <= (mBarPointerHaloRadius + mBarLength)) {
                        mBarPointerPosition = Math.round(dimen);
                        setSVFromCoordinates(dimen);
                        setColor(mHSVColor);
                        invalidate();

                        // Touch event happens on the start point or to the left of it.
                    } else if (dimen < mBarPointerHaloRadius) {
                        mBarPointerPosition = mBarPointerHaloRadius;
                        setSV(0);
                        setColor(mHSVColor);
                        invalidate();

                        // Touch event happens to the right of the end point
                    } else if (dimen > (mBarPointerHaloRadius - mBarLength)) {
                        mBarPointerPosition = mBarPointerHaloRadius + mBarLength;
                        setSV(1);
                        setColor(mHSVColor);
                        invalidate();
                    }
                }
                int rgbCol = getDisplayColor(mHSVColor);
                if (onOmniChangedListener != null && oldChangedListenerColor != rgbCol) {
                    onOmniChangedListener.onOmniChanged(rgbCol);
                    oldChangedListenerColor = rgbCol;
                }

                break;
            case MotionEvent.ACTION_UP:
                mIsMovingPointer = false;
                break;
        }
        return true;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        // Fill the rectangle instance based on orientation
        int x1, y1;
        if (mBarIsHorizontal) {
            x1 = (mBarLength + mBarPointerHaloRadius);
            y1 = mBarThickness;
            mBarLength = w - (mBarPointerHaloRadius * 2);
            mBarRect.set(
                    mBarPointerHaloRadius,
                    (mBarPointerHaloRadius - (mBarThickness / 2f)),
                    (mBarLength + (mBarPointerHaloRadius)),
                    (mBarPointerHaloRadius + (mBarThickness / 2f)));
        } else {
            x1 = mBarThickness;
            y1 = (mBarLength + mBarPointerHaloRadius);
            mBarLength = h - (mBarPointerHaloRadius * 2);
            mBarRect.set(
                    (mBarPointerHaloRadius - (mBarThickness / 2f)),
                    mBarPointerHaloRadius,
                    (mBarPointerHaloRadius + (mBarThickness / 2f)),
                    (mBarLength + (mBarPointerHaloRadius)));
        }

        // Update variables that depend of mBarLength.
        if (!isInEditMode()) {
            shader =
                    new LinearGradient(
                            mBarPointerHaloRadius,
                            0,
                            x1,
                            y1,
                            new int[]{getDisplayColor(mHSVColor, 0), getDisplayColor(mHSVColor, 1)},
                            null,
                            Shader.TileMode.CLAMP);
        } else {
            shader =
                    new LinearGradient(
                            mBarPointerHaloRadius,
                            0,
                            x1,
                            y1,
                            new int[]{Color.WHITE, 0xff81ff00},
                            null,
                            Shader.TileMode.CLAMP);
            Color.colorToHSV(0xff81ff00, mHSVColor);
        }

        mBarPaint.setShader(shader);
        mPosToSVFactor = 1 / ((float) mBarLength);
        mSVToPosFactor = ((float) mBarLength) / 1;

        if (!isInEditMode()) {
            mBarPointerPosition =
                    Math.round((mSVToPosFactor * mHSVColor[mBarType]) + mBarPointerHaloRadius);
        } else {
            mBarPointerPosition = mBarLength + mBarPointerHaloRadius;
        }
    }

    @Override
    protected void onDraw(@NotNull Canvas canvas) {
        // Draw the bar.
        canvas.drawRect(mBarRect, mBarPaint);

        // Calculate the center of the pointer.
        int cX, cY;
        if (mBarIsHorizontal) {
            cX = mBarPointerPosition;
            cY = mBarPointerHaloRadius;
        } else {
            cX = mBarPointerHaloRadius;
            cY = mBarPointerPosition;
        }

        // Draw the pointer halo.
        canvas.drawCircle(cX, cY, mBarPointerHaloRadius, mBarPointerHaloPaint);
        // Draw the pointer.
        canvas.drawCircle(cX, cY, mBarPointerRadius, mBarPointerPaint);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();

        Bundle state = new Bundle();
        state.putParcelable(STATE_PARENT, superState);
        state.putInt(STATE_ALPHA, mAlpha);
        state.putFloatArray(STATE_COLOR, mHSVColor);

        return state;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        Bundle savedState = (Bundle) state;

        Parcelable superState = savedState.getParcelable(STATE_PARENT);
        super.onRestoreInstanceState(superState);

        float[] floats = savedState.getFloatArray(STATE_COLOR);
        if (floats == null) floats = new float[3];
        initializeColor(savedState.getInt(STATE_ALPHA), floats);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int intrinsicSize = mPreferredBarLength + (mBarPointerHaloRadius * 2);

        // Variable orientation
        int measureSpec;
        if (mBarIsHorizontal) {
            measureSpec = widthMeasureSpec;
        } else {
            measureSpec = heightMeasureSpec;
        }
        int lengthMode = MeasureSpec.getMode(measureSpec);
        int lengthSize = MeasureSpec.getSize(measureSpec);

        int length;
        if (lengthMode == MeasureSpec.EXACTLY) {
            length = lengthSize;
        } else if (lengthMode == MeasureSpec.AT_MOST) {
            length = Math.min(intrinsicSize, lengthSize);
        } else {
            length = intrinsicSize;
        }

        int barPointerHaloRadiusx2 = mBarPointerHaloRadius * 2;
        mBarLength = length - barPointerHaloRadiusx2;
        if (!mBarIsHorizontal) {
            setMeasuredDimension(barPointerHaloRadiusx2, (mBarLength + barPointerHaloRadiusx2));
        } else {
            setMeasuredDimension((mBarLength + barPointerHaloRadiusx2), barPointerHaloRadiusx2);
        }
    }

    public void initializeColor(int alpha, float @NotNull [] color) {
        mAlpha = alpha;
        mBarPointerPosition = Math.round(((mSVToPosFactor * color[mBarType])) + mBarPointerHaloRadius);
        setColor(color, true);
    }

    private void setColor(float[] color, boolean initialize) {
        int x1, y1;
        if (mBarIsHorizontal) {
            x1 = (mBarLength + mBarPointerHaloRadius);
            y1 = mBarThickness;
        } else {
            x1 = mBarThickness;
            y1 = (mBarLength + mBarPointerHaloRadius);
        }
        System.arraycopy(color, 0, mHSVColor, 0, 3);

        shader =
                new LinearGradient(
                        mBarPointerHaloRadius,
                        0,
                        x1,
                        y1,
                        new int[]{getDisplayColor(color, 0), getDisplayColor(color, 1)},
                        null,
                        Shader.TileMode.CLAMP);
        mBarPaint.setShader(shader);

        mBarPointerPaint.setColor(getDisplayColor(color));
        if (!initialize) {
            if (mPicker != null) {
                mPicker.setColor(mAlpha, mHSVColor, mBarType);
            }
        }
        invalidate();
    }

    private int getDisplayColor(float[] color) {
        return getDisplayColor(color, color[mBarType]);
    }

    private int getDisplayColor(float[] color, float omni) {
        float[] col = new float[3];
        System.arraycopy(color, 0, col, 0, 3);
        col[mBarType] = omni;
        return Color.HSVToColor(mAlpha, col);
    }

    private void setColor(float[] color) {
        setColor(color, false);
    }

    private void setSV(float omni) {
        mHSVColor[mBarType] = omni;
    }

    private void setSVFromCoordinates(float coord) {
        coord = coord - mBarPointerHaloRadius;
        if (coord < 0) {
            coord = 0;
        } else if (coord > mBarLength) {
            coord = mBarLength;
        }
        float omni = mPosToSVFactor * coord;
        setSV(omni);
    }

    public void setColorPicker(ColorPicker picker) {
        mPicker = picker;
    }

    private void logHSV(String source, float @NotNull [] mHSVColor) {
        Log.d(TAG, source + ": " + mHSVColor[0] + "/" + mHSVColor[1] + "/" + mHSVColor[2]);
    }

    public void setBarType(int type) {
        this.mBarType = type;
    }

    public void setBarThickness(int barThickness) {
        this.mBarThickness = barThickness;
    }

    public void setBarLength(int barLength) {
        this.mBarLength = barLength;
        mPreferredBarLength = barLength;
    }

    public void setBarPointerRadius(int barPointerRadius) {
        this.mBarPointerRadius = barPointerRadius;
    }

    public void setBarPointerHaloRadius(int barPointerHaloRadius) {
        this.mBarPointerHaloRadius = barPointerHaloRadius;
    }

    public void setBarPointerHaloColor(int barPointerHaloColor) {
        this.mBarPointerHaloColor = barPointerHaloColor;
        mBarPointerHaloPaint.setColor(mBarPointerHaloColor);
    }

    public interface OnOmniChangedListener {

        void onOmniChanged(int omni);
    }
}
