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
import android.view.MotionEvent;
import android.view.View;

import com.github.koston.preference.R;

import org.jetbrains.annotations.NotNull;

@SuppressWarnings("unused")
public class OpacityBar extends View {

    private static final String STATE_PARENT = "parent";
    private static final String STATE_COLOR = "color";
    private static final String STATE_OPACITY = "opacity";

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

    private int mColor;
    private int mAlpha;
    private float[] mHSVColor = new float[3];

    private float mPosToOpacityFactor;
    private float mOpacityToPosFactor;

    private OnOpacityChangedListener onOpacityChangedListener;
    private int oldChangedListenerOpacity;

    private ColorPicker mPicker = null;

    public OpacityBar(Context context) {
        super(context);
        init(null, 0);
    }

    public OpacityBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    public OpacityBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(attrs, defStyle);
    }

    private void init(AttributeSet attrs, int defStyle) {
        @SuppressLint("CustomViewStyleable") final TypedArray a =
                getContext().obtainStyledAttributes(attrs, R.styleable.ColorPickerBars, defStyle, 0);
        final Resources b = getContext().getResources();

        mBarThickness =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barThickness,
                        b.getDimensionPixelSize(R.dimen.defaultBarThickness));
        mBarLength =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barLength,
                        b.getDimensionPixelSize(R.dimen.defaultBarLength));
        mPreferredBarLength = mBarLength;
        mBarPointerRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barPointerRadius,
                        b.getDimensionPixelSize(R.dimen.defaultBarPointerRadius));
        mBarPointerHaloRadius =
                a.getDimensionPixelSize(
                        R.styleable.ColorPickerBars_barPointerHaloRadius,
                        b.getDimensionPixelSize(R.dimen.defaultBarPointerHaloRadius));
        mBarPointerHaloColor =
                a.getColor(
                        R.styleable.ColorPickerBars_barPointerHaloColor,
                        b.getColor(R.color.defaultPointerHaloColor));
        mBarIsHorizontal = a.getBoolean(R.styleable.ColorPickerBars_barOrientationHorizontal, true);

        a.recycle();

        mBarPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBarPaint.setShader(shader);

        mBarPointerPosition = mBarLength + mBarPointerHaloRadius;

        mBarPointerHaloPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBarPointerHaloPaint.setColor(mBarPointerHaloColor);

        mBarPointerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBarPointerPaint.setColor(0xff81ff00);

        mPosToOpacityFactor = 0xFF / ((float) mBarLength);
        mOpacityToPosFactor = ((float) mBarLength) / 0xFF;
    }

    public OnOpacityChangedListener getOnOpacityChangedListener() {
        return this.onOpacityChangedListener;
    }

    public void setOnOpacityChangedListener(OnOpacityChangedListener listener) {
        this.onOpacityChangedListener = listener;
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
                    calculateColor(Math.round(dimen));
                    mBarPointerPaint.setColor(mColor);
                    invalidate();
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (mIsMovingPointer) {
                    // Move the the pointer on the bar.
                    if (dimen >= mBarPointerHaloRadius && dimen <= (mBarPointerHaloRadius + mBarLength)) {
                        mBarPointerPosition = Math.round(dimen);
                        calculateColor(Math.round(dimen));
                        setOpacity(mColor);
                        invalidate();
                    } else if (dimen < mBarPointerHaloRadius) {
                        mBarPointerPosition = mBarPointerHaloRadius;
                        mColor = lowerBoundColor(mHSVColor);
                        setOpacity(mColor);
                        invalidate();
                    } else if (dimen > (mBarPointerHaloRadius + mBarLength)) {
                        mBarPointerPosition = mBarPointerHaloRadius + mBarLength;
                        mColor = upperBoundColor(mHSVColor);
                        setOpacity(mColor);
                        invalidate();
                    }
                }
                if (onOpacityChangedListener != null && oldChangedListenerOpacity != getOpacity()) {
                    onOpacityChangedListener.onOpacityChanged(getOpacity());
                    oldChangedListenerOpacity = getOpacity();
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
                            new int[]{Color.HSVToColor(0x00, mHSVColor), Color.HSVToColor(0xFF, mHSVColor)},
                            null,
                            Shader.TileMode.CLAMP);
        } else {
            shader =
                    new LinearGradient(
                            mBarPointerHaloRadius,
                            0,
                            x1,
                            y1,
                            new int[]{0x0081ff00, 0xff81ff00},
                            null,
                            Shader.TileMode.CLAMP);
            Color.colorToHSV(0xff81ff00, mHSVColor);
        }

        mBarPaint.setShader(shader);
        mPosToOpacityFactor = 0xFF / ((float) mBarLength);
        mOpacityToPosFactor = ((float) mBarLength) / 0xFF;

        float[] hsvColor = new float[3];
        Color.colorToHSV(mColor, hsvColor);

        if (!isInEditMode()) {
            mBarPointerPosition = Math.round((mOpacityToPosFactor * mAlpha) + mBarPointerHaloRadius);
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
        state.putFloatArray(STATE_COLOR, mHSVColor);
        state.putInt(STATE_OPACITY, getOpacity());

        return state;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        Bundle savedState = (Bundle) state;

        Parcelable superState = savedState.getParcelable(STATE_PARENT);
        super.onRestoreInstanceState(superState);

        initializeColor(savedState.getInt(STATE_OPACITY), savedState.getFloatArray(STATE_COLOR));
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

    public void initializeColor(int alpha, float[] color) {
        mAlpha = alpha;
        mBarPointerPosition = Math.round((mOpacityToPosFactor * alpha)) + mBarPointerHaloRadius;
        setColor(color, true);
    }

    private void calculateColor(int coord) {
        coord = coord - mBarPointerHaloRadius;
        if (coord < 0) {
            coord = 0;
        } else if (coord > mBarLength) {
            coord = mBarLength;
        }

        mAlpha = Math.round(mPosToOpacityFactor * coord);
        mColor = Color.HSVToColor(mAlpha, mHSVColor);
        if (mAlpha > 250) {
            mColor = upperBoundColor(mHSVColor);
        } else if (mAlpha < 5) {
            mColor = lowerBoundColor(mHSVColor);
        }
    }

    private int lowerBoundColor(float[] color) {
        return Color.HSVToColor(0, color);
    }

    private int upperBoundColor(float[] color) {
        return Color.HSVToColor(255, color);
    }

    public int getOpacity() {
        int opacity = Math.round((mPosToOpacityFactor * (mBarPointerPosition - mBarPointerHaloRadius)));
        if (opacity < 5) {
            return 0x00;
        } else if (opacity > 250) {
            return 0xFF;
        } else {
            return opacity;
        }
    }

    private void setOpacity(int color) {
        mAlpha = Color.alpha(color);
        setColor(mHSVColor, false);
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
        mColor = Color.HSVToColor(color);
        mHSVColor = color;
        shader =
                new LinearGradient(
                        mBarPointerHaloRadius,
                        0,
                        x1,
                        y1,
                        new int[]{lowerBoundColor(mHSVColor), mColor},
                        null,
                        Shader.TileMode.CLAMP);
        mBarPaint.setShader(shader);

        mBarPointerPaint.setColor(mColor);

        if (!initialize) {
            if (mPicker != null) {
                mPicker.setColor(mAlpha, mHSVColor, ColorPicker.TYPE_OPACITY);
            }
        }
        invalidate();
    }

    public int getColor() {
        return mColor;
    }

    public void setColorPicker(ColorPicker picker) {
        mPicker = picker;
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

    public interface OnOpacityChangedListener {

        void onOpacityChanged(int opacity);
    }
}
