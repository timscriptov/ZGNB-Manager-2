package com.zengge.nbmanager.utils;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.provider.Settings;
import android.telephony.TelephonyManager;

import androidx.appcompat.app.AlertDialog;

import com.zengge.nbmanager.Features;
import com.zengge.nbmanager.R;

import org.jetbrains.annotations.NotNull;

import java.util.UUID;

public class DeviceInfo {
    @SuppressLint({"MissingPermission", "HardwareIds"})
    public static void systemInfo(@NotNull Context context) {
        StringBuilder info = new StringBuilder();
        TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        @SuppressLint("HardwareIds") String androidId = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
        @SuppressLint("HardwareIds") String dui = new UUID(androidId.hashCode(),
                ((long) tm.getDeviceId().hashCode() << 64 | tm.hashCode()) & tm.toString().hashCode()).toString();
        dui = dui.replaceAll("-", "");
        info.append("Model：").append(Build.MODEL).append("\n");
        info.append("Manufacturer：").append(Build.MANUFACTURER).append("\n");
        info.append("Android Version：").append(Build.VERSION.RELEASE).append("\n");
        info.append("Android SDK Version：").append(Build.VERSION.SDK_INT).append("\n");
        info.append("CPU ABI：").append(Build.CPU_ABI).append(" / ").append(Build.CPU_ABI2).append("\n");
        info.append("Serial：").append(Build.SERIAL).append("\n");
        info.append("Hardware：").append(Build.HARDWARE).append("\n");
        info.append("基带版本：").append(Build.getRadioVersion()).append("\n");
        info.append("BootLoader Version：").append(Build.BOOTLOADER).append("\n");
        info.append("Device ID：").append(tm.getDeviceId()).append("\n");
        info.append("Machine code：").append(dui).append("\n");
        info.append("App signature：").append(Features.compressStrToInt(getPkgSign(context)));
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(context.getString(R.string.system_info));
        builder.setMessage(info.toString());
        builder.setNeutralButton(R.string.btn_ok, null);
        builder.show();
    }

    public static @NotNull String getPkgSign(Context ctx) {
        try {
            PackageManager pm = ctx.getPackageManager();
            @SuppressLint("PackageManagerGetSignatures") PackageInfo pi = pm.getPackageInfo(ctx.getPackageName(), PackageManager.GET_SIGNATURES);
            return new String(pi.signatures[0].toChars());
        } catch (Exception e) {
            e.printStackTrace();
            return "NULL";
        }
    }
}
