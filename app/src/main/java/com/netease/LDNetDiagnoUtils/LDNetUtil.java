package com.netease.LDNetDiagnoUtils;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.UnknownHostException;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

@SuppressLint("DefaultLocale")
public class LDNetUtil {

    public static final String OPEN_IP = "";
    public static final String OPERATOR_URL = "";

    public static final String NETWORKTYPE_INVALID = "UNKNOWN";
    public static final String NETWORKTYPE_WAP = "WAP";
    public static final String NETWORKTYPE_WIFI = "WIFI";

    @SuppressWarnings({"deprecation"})
    public static String getNetWorkType(@NotNull Context context) {
        String mNetWorkType = null;
        ConnectivityManager manager = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        if (manager == null) {
            return "ConnectivityManager not found";
        }
        NetworkInfo networkInfo = manager.getActiveNetworkInfo();
        if (networkInfo != null && networkInfo.isConnected()) {
            String type = networkInfo.getTypeName();
            if (type.equalsIgnoreCase("WIFI")) {
                mNetWorkType = NETWORKTYPE_WIFI;
            } else if (type.equalsIgnoreCase("MOBILE")) {
                String proxyHost = android.net.Proxy.getDefaultHost();
                if (TextUtils.isEmpty(proxyHost)) {
                    mNetWorkType = mobileNetworkType(context);
                } else {
                    mNetWorkType = NETWORKTYPE_WAP;
                }
            }
        } else {
            mNetWorkType = NETWORKTYPE_INVALID;
        }
        return mNetWorkType;
    }


    public static @NotNull Boolean isNetworkConnected(@NotNull Context context) {
        ConnectivityManager manager = (ConnectivityManager) context
                .getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        if (manager == null) {
            return false;
        }
        NetworkInfo networkinfo = manager.getActiveNetworkInfo();
        if (networkinfo == null || !networkinfo.isAvailable()) {
            return false;
        }
        return true;
    }

    public static @NotNull String getMobileOperator(@NotNull Context context) {
        TelephonyManager telManager = (TelephonyManager) context
                .getSystemService(Context.TELEPHONY_SERVICE);
        if (telManager == null)
            return "Unknown service provider";
        String operator = telManager.getSimOperator();
        if (operator != null) {
            if (operator.equals("46000") || operator.equals("46002")
                    || operator.equals("46007")) {
                return "China Mobile";
            } else if (operator.equals("46001")) {
                return "China Unicom";
            } else if (operator.equals("46003")) {
                return "China Telecom";
            }
        }
        return "Unknown service provider";
    }

    /**
     * IP(wifi)
     */
    public static @NotNull String getLocalIpByWifi(@NotNull Context context) {
        WifiManager wifiManager = (WifiManager) context.getApplicationContext()
                .getSystemService(Context.WIFI_SERVICE);
        if (wifiManager == null) {
            return "wifiManager not found";
        }
        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
        if (wifiInfo == null) {
            return "wifiInfo not found";
        }
        int ipAddress = wifiInfo.getIpAddress();
        return String.format("%d.%d.%d.%d", (ipAddress & 0xff),
                (ipAddress >> 8 & 0xff), (ipAddress >> 16 & 0xff),
                (ipAddress >> 24 & 0xff));
    }

    /**
     * IP(2G/3G/4G)
     */
    public static @Nullable String getLocalIpBy3G() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface
                    .getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr
                        .hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress()
                            && inetAddress instanceof Inet4Address) {
                        // if (!inetAddress.isLoopbackAddress() && inetAddress
                        // instanceof Inet6Address) {
                        return inetAddress.getHostAddress().toString();
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * wifi
     */
    public static String pingGateWayInWifi(@NotNull Context context) {
        String gateWay = null;
        WifiManager wifiManager = (WifiManager) context.getApplicationContext()
                .getSystemService(Context.WIFI_SERVICE);
        if (wifiManager == null) {
            return "wifiManager not found";
        }
        DhcpInfo dhcpInfo = wifiManager.getDhcpInfo();
        if (dhcpInfo != null) {
            int tmp = dhcpInfo.gateway;
            gateWay = String.format("%d.%d.%d.%d", (tmp & 0xff), (tmp >> 8 & 0xff),
                    (tmp >> 16 & 0xff), (tmp >> 24 & 0xff));
        }
        return gateWay;
    }

    /**
     * DNS
     */
    public static @NotNull String getLocalDns(String dns) {
        Process process = null;
        String str = "";
        BufferedReader reader = null;
        try {
            process = Runtime.getRuntime().exec("getprop net." + dns);
            reader = new BufferedReader(new InputStreamReader(
                    process.getInputStream()));
            String line = null;
            while ((line = reader.readLine()) != null) {
                str += line;
            }
            reader.close();
            process.waitFor();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
                process.destroy();
            } catch (Exception e) {
            }
        }
        return str.trim();
    }

    /**
     *
     */
    public static @NotNull Map<String, Object> getDomainIp(String _dormain) {
        Map<String, Object> map = new HashMap<String, Object>();
        long start = 0;
        long end = 0;
        String time = null;
        InetAddress[] remoteInet = null;
        try {
            start = System.currentTimeMillis();
            remoteInet = InetAddress.getAllByName(_dormain);
            if (remoteInet != null) {
                end = System.currentTimeMillis();
                time = (end - start) + "";
            }
        } catch (UnknownHostException e) {
            end = System.currentTimeMillis();
            time = (end - start) + "";
            remoteInet = null;
            e.printStackTrace();
        } finally {
            map.put("remoteInet", remoteInet);
            map.put("useTime", time);
        }
        return map;
    }

    @SuppressLint("MissingPermission")
    private static @NotNull String mobileNetworkType(@NotNull Context context) {
        TelephonyManager telephonyManager = (TelephonyManager) context
                .getSystemService(Context.TELEPHONY_SERVICE);
        if (telephonyManager == null) {
            return "TM==null";
        }
        switch (telephonyManager.getNetworkType()) {
            case TelephonyManager.NETWORK_TYPE_1xRTT:// ~ 50-100 kbps
                return "2G";
            case TelephonyManager.NETWORK_TYPE_CDMA:// ~ 14-64 kbps
                return "2G";
            case TelephonyManager.NETWORK_TYPE_EDGE:// ~ 50-100 kbps
                return "2G";
            case TelephonyManager.NETWORK_TYPE_EVDO_0:// ~ 400-1000 kbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_EVDO_A:// ~ 600-1400 kbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_GPRS:// ~ 100 kbps
                return "2G";
            case TelephonyManager.NETWORK_TYPE_HSDPA:// ~ 2-14 Mbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_HSPA:// ~ 700-1700 kbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_HSUPA: // ~ 1-23 Mbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_UMTS:// ~ 400-7000 kbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_EHRPD:// ~ 1-2 Mbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_EVDO_B: // ~ 5 Mbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_HSPAP:// ~ 10-20 Mbps
                return "3G";
            case TelephonyManager.NETWORK_TYPE_IDEN:// ~25 kbps
                return "2G";
            case TelephonyManager.NETWORK_TYPE_LTE:// ~ 10+ Mbps
                return "4G";
            case TelephonyManager.NETWORK_TYPE_UNKNOWN:
                return "UNKNOWN";
            default:
                return "4G";
        }
    }

    /**
     *
     */
    public static String getStringFromStream(@NotNull InputStream is) {
        byte[] bytes = new byte[1024];
        int len = 0;
        String res = "";
        try {
            while ((len = is.read(bytes)) != -1) {
                res = res + new String(bytes, 0, len, "gbk");
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return res;
    }
}
