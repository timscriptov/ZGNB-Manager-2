package com.netease.LDNetDiagnoService;

import android.content.Context;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.netease.LDNetDiagnoService.LDNetPing.LDNetPingListener;
import com.netease.LDNetDiagnoService.LDNetSocket.LDNetSocketListener;
import com.netease.LDNetDiagnoService.LDNetTraceRoute.LDNetTraceRouteListener;
import com.netease.LDNetDiagnoUtils.LDNetUtil;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

public class LDNetDiagnoService extends
        LDNetAsyncTaskEx<String, String, String> implements LDNetPingListener,
        LDNetTraceRouteListener, LDNetSocketListener {
    private static final int CORE_POOL_SIZE = 1;// 4
    private static final int MAXIMUM_POOL_SIZE = 1;// 10
    private static final int KEEP_ALIVE = 10;// 10
    private static final BlockingQueue<Runnable> sWorkQueue = new LinkedBlockingQueue<Runnable>(
            2);// 2
    private static final ThreadFactory sThreadFactory = new ThreadFactory() {
        private final AtomicInteger mCount = new AtomicInteger(1);

        @Override
        public @NotNull Thread newThread(Runnable r) {
            Thread t = new Thread(r, "Trace #" + mCount.getAndIncrement());
            t.setPriority(Thread.MIN_PRIORITY);
            return t;
        }
    };
    private static ThreadPoolExecutor sExecutor = null;
    private final StringBuilder _logInfo = new StringBuilder(256);
    private String _appCode;
    private String _appName;
    private String _appVersion;
    private String _UID;
    private String _deviceID;
    private String _dormain;
    private String _carrierName;
    private String _ISOCountryCode;
    private String _MobileCountryCode;
    private String _MobileNetCode;
    private boolean _isNetConnected;
    private boolean _isDomainParseOk;
    private boolean _isSocketConnected;
    private Context _context;
    private String _netType;
    private String _localIp;
    private String _gateWay;
    private String _dns1;
    private String _dns2;
    private InetAddress[] _remoteInet;
    private List<String> _remoteIpList;
    private LDNetSocket _netSocker;
    private LDNetPing _netPinger;
    private LDNetTraceRoute _traceRouter;
    private boolean _isRunning;
    private LDNetDiagnoListener _netDiagnolistener;
    private boolean _isUseJNICConn = false;
    private boolean _isUseJNICTrace = true;
    private TelephonyManager _telManager = null;

    public LDNetDiagnoService() {
        super();
    }

    /**
     * @param theAppCode
     * @param theDeviceID
     * @param theUID
     * @param theDormain
     */
    public LDNetDiagnoService(@NotNull Context context, String theAppCode,
                              String theAppName, String theAppVersion, String theUID,
                              String theDeviceID, String theDormain, String theCarrierName,
                              String theISOCountryCode, String theMobileCountryCode,
                              String theMobileNetCode, LDNetDiagnoListener theListener) {
        super();
        this._context = context;
        this._appCode = theAppCode;
        this._appName = theAppName;
        this._appVersion = theAppVersion;
        this._UID = theUID;
        this._deviceID = theDeviceID;
        this._dormain = theDormain;
        this._carrierName = theCarrierName;
        this._ISOCountryCode = theISOCountryCode;
        this._MobileCountryCode = theMobileCountryCode;
        this._MobileNetCode = theMobileNetCode;
        this._netDiagnolistener = theListener;
        //
        this._isRunning = false;
        _remoteIpList = new ArrayList<String>();
        _telManager = (TelephonyManager) context
                .getSystemService(Context.TELEPHONY_SERVICE);
        sExecutor = new ThreadPoolExecutor(CORE_POOL_SIZE, MAXIMUM_POOL_SIZE,
                KEEP_ALIVE, TimeUnit.SECONDS, sWorkQueue, sThreadFactory);

    }

    @Override
    protected String doInBackground(String... params) {
        if (this.isCancelled())
            return null;
        // TODO Auto-generated method stub
        return this.startNetDiagnosis();
    }

    @Override
    protected void onPostExecute(String result) {
        if (this.isCancelled())
            return;
        super.onPostExecute(result);

        recordStepInfo("\nEnd of network diagnosis\n");
        this.stopNetDialogsis();
        if (_netDiagnolistener != null) {
            _netDiagnolistener.OnNetDiagnoFinished(_logInfo.toString());
        }
    }

    @Override
    protected void onProgressUpdate(String... values) {
        if (this.isCancelled())
            return;
        // TODO Auto-generated method stub
        super.onProgressUpdate(values);
        if (_netDiagnolistener != null) {
            _netDiagnolistener.OnNetDiagnoUpdated(values[0]);
        }
    }

    @Override
    protected void onCancelled() {
        this.stopNetDialogsis();
    }

    /**
     *
     */
    public String startNetDiagnosis() {
        if (TextUtils.isEmpty(this._dormain))
            return "";
        this._isRunning = true;
        this._logInfo.setLength(0);
        recordStepInfo("Start diagnosis...\n");
        recordCurrentAppVersion();
        recordLocalNetEnvironmentInfo();

        if (_isNetConnected) {
            recordStepInfo("\nStart TCP connection test...");
            _netSocker = LDNetSocket.getInstance();
            _netSocker._remoteInet = _remoteInet;
            _netSocker._remoteIpList = _remoteIpList;
            _netSocker.initListener(this);
            _netSocker.isCConn = this._isUseJNICConn;
            _isSocketConnected = _netSocker.exec(_dormain);


            if (!(_isNetConnected && _isDomainParseOk && _isSocketConnected)) {
                recordStepInfo("\nStart ping...");
                _netPinger = new LDNetPing(this, 4);
                recordStepInfo("ping...127.0.0.1");
                _netPinger.exec("127.0.0.1", false);
                recordStepInfo("ping localhost IP..." + _localIp);
                _netPinger.exec(_localIp, false);
                if (LDNetUtil.NETWORKTYPE_WIFI.equals(_netType)) {
                    recordStepInfo("ping localhost gateway..." + _gateWay);
                    _netPinger.exec(_gateWay, false);
                }
                recordStepInfo("ping localhost DNS1..." + _dns1);
                _netPinger.exec(_dns1, false);
                recordStepInfo("ping localhost DNS2..." + _dns2);
                _netPinger.exec(_dns2, false);
            }

            if (_netPinger == null) {
                _netPinger = new LDNetPing(this, 4);
            }

            recordStepInfo("\nStart traceroute...");
            _traceRouter = LDNetTraceRoute.getInstance();
            _traceRouter.initListenter(this);
            _traceRouter.isCTrace = this._isUseJNICTrace;
            _traceRouter.startTraceRoute(_dormain);
            return _logInfo.toString();
        } else {
            recordStepInfo("\n\nThe current host is not connected to the Internet, please check the network!");
            return _logInfo.toString();
        }
    }

    /**
     *
     */
    public void stopNetDialogsis() {
        if (_isRunning) {
            if (_netSocker != null) {
                _netSocker.resetInstance();
                _netSocker = null;
            }

            if (_netPinger != null) {
                _netPinger = null;
            }
            if (_traceRouter != null) {
                _traceRouter.resetInstance();
                _traceRouter = null;
            }
            cancel(true);
            if (sExecutor != null && !sExecutor.isShutdown()) {
                sExecutor.shutdown();
                sExecutor = null;
            }

            _isRunning = false;
        }
    }

    /**
     * JNICTraceRoute
     *
     * @param use
     */
    public void setIfUseJNICConn(boolean use) {
        this._isUseJNICConn = use;
    }

    /**
     * JNICTraceRoute
     *
     * @param use
     */
    public void setIfUseJNICTrace(boolean use) {
        this._isUseJNICTrace = use;
    }

    /**
     * loginInfo；
     */
    public void printLogInfo() {
        System.out.print(_logInfo);
    }

    /**
     * stepInfo，
     *
     * @param stepInfo
     */
    private void recordStepInfo(String stepInfo) {
        _logInfo.append(stepInfo + "\n");
        publishProgress(stepInfo + "\n");
    }

    /**
     * traceroute
     */
    @Override
    public void OnNetTraceFinished() {
    }

    @Override
    public void OnNetTraceUpdated(String log) {
        if (log == null) {
            return;
        }
        if (this._traceRouter != null && this._traceRouter.isCTrace) {
            if (log.contains("ms") || log.contains("***")) {
                log += "\n";
            }
            _logInfo.append(log);
            publishProgress(log);
        } else {
            this.recordStepInfo(log);
        }
    }

    /**
     * socket
     */
    @Override
    public void OnNetSocketFinished(String log) {
        _logInfo.append(log);
        publishProgress(log);
    }

    /**
     * socket
     */
    @Override
    public void OnNetSocketUpdated(String log) {
        _logInfo.append(log);
        publishProgress(log);
    }

    private void recordCurrentAppVersion() {
        recordStepInfo("Application code:\t" + _appCode);
        recordStepInfo("Application name:\t" + this._appName);
        recordStepInfo("Application version:\t" + this._appVersion);

        recordStepInfo("Device model:\t" + android.os.Build.MANUFACTURER + ":"
                + android.os.Build.BRAND + ":" + android.os.Build.MODEL);
        recordStepInfo("System version:\t" + android.os.Build.VERSION.RELEASE);


        if (TextUtils.isEmpty(_carrierName)) {
            _carrierName = LDNetUtil.getMobileOperator(_context);
        }
        recordStepInfo("Telecom service provider:\t" + _carrierName);

        if (_telManager != null && TextUtils.isEmpty(_ISOCountryCode)) {
            _ISOCountryCode = _telManager.getNetworkCountryIso();
        }
        recordStepInfo("ISOCountryCode:\t" + _ISOCountryCode);

        if (_telManager != null && TextUtils.isEmpty(_MobileCountryCode)) {
            String tmp = _telManager.getNetworkOperator();
            if (tmp.length() >= 3) {
                _MobileCountryCode = tmp.substring(0, 3);
            }
            if (tmp.length() >= 5) {
                _MobileNetCode = tmp.substring(3, 5);
            }
        }
        recordStepInfo("MobileCountryCode:\t" + _MobileCountryCode);
        recordStepInfo("MobileNetworkCode:\t" + _MobileNetCode + "\n");
    }

    /**
     *
     */
    private void recordLocalNetEnvironmentInfo() {
        recordStepInfo("Diagnosing domain " + _dormain + "...");


        if (LDNetUtil.isNetworkConnected(_context)) {
            _isNetConnected = true;
            recordStepInfo("Connected to Internet");
        } else {
            _isNetConnected = false;
            recordStepInfo("Disconnected from Internet");
        }


        _netType = LDNetUtil.getNetWorkType(_context);
        recordStepInfo("Current network type:\t" + _netType);
        if (_isNetConnected) {
            if (LDNetUtil.NETWORKTYPE_WIFI.equals(_netType)) { // wifi：ip，：ip
                _localIp = LDNetUtil.getLocalIpByWifi(_context);
                _gateWay = LDNetUtil.pingGateWayInWifi(_context);
            } else {
                _localIp = LDNetUtil.getLocalIpBy3G();
            }
            recordStepInfo("Localhost IP:\t" + _localIp);
        } else {
            recordStepInfo("Localhost IP:\t" + "127.0.0.1");
        }
        if (_gateWay != null) {
            recordStepInfo("Localhost gateway:\t" + this._gateWay);
        }


        if (_isNetConnected) {
            _dns1 = LDNetUtil.getLocalDns("dns1");
            _dns2 = LDNetUtil.getLocalDns("dns2");
            recordStepInfo("Localhost DNS:\t" + this._dns1 + "," + this._dns2);
        } else {
            recordStepInfo("Localhost DNS:\t" + "0.0.0.0" + "," + "0.0.0.0");
        }


        if (_isNetConnected) {
            recordStepInfo("Remote domain:\t" + this._dormain);
            _isDomainParseOk = parseDomain(this._dormain);
        }
    }

    /**
     *
     */
    private boolean parseDomain(String _dormain) {
        boolean flag = false;
        int len = 0;
        String ipString = "";
        Map<String, Object> map = LDNetUtil.getDomainIp(_dormain);
        String useTime = (String) map.get("useTime");
        _remoteInet = (InetAddress[]) map.get("remoteInet");
        String timeShow = null;
        if (Integer.parseInt(useTime) > 5000) {
            timeShow = " (" + Integer.parseInt(useTime) / 1000 + "s)";
        } else {
            timeShow = " (" + useTime + "ms)";
        }
        if (_remoteInet != null) {
            len = _remoteInet.length;
            for (int i = 0; i < len; i++) {
                _remoteIpList.add(_remoteInet[i].getHostAddress());
                ipString += _remoteInet[i].getHostAddress() + ",";
            }
            ipString = ipString.substring(0, ipString.length() - 1);
            recordStepInfo("DNS resolution result:\t" + ipString + timeShow);
            flag = true;
        } else {
            if (Integer.parseInt(useTime) > 10000) {
                map = LDNetUtil.getDomainIp(_dormain);
                useTime = (String) map.get("useTime");
                _remoteInet = (InetAddress[]) map.get("remoteInet");
                if (Integer.parseInt(useTime) > 5000) {
                    timeShow = " (" + Integer.parseInt(useTime) / 1000 + "s)";
                } else {
                    timeShow = " (" + useTime + "ms)";
                }
                if (_remoteInet != null) {
                    len = _remoteInet.length;
                    for (int i = 0; i < len; i++) {
                        _remoteIpList.add(_remoteInet[i].getHostAddress());
                        ipString += _remoteInet[i].getHostAddress() + ",";
                    }
                    ipString = ipString.substring(0, ipString.length() - 1);
                    recordStepInfo("DNS resolution result:\t" + ipString + timeShow);
                    flag = true;
                } else {
                    recordStepInfo("DNS resolution result:\t" + "Failed" + timeShow);
                }
            } else {
                recordStepInfo("DNS resolution result:\t" + "Failed" + timeShow);
            }
        }
        return flag;
    }

    /**
     *
     */
    private String requestOperatorInfo() {
        String res = null;
        String url = LDNetUtil.OPERATOR_URL;
        HttpURLConnection conn = null;
        URL Operator_url;
        try {
            Operator_url = new URL(url);
            conn = (HttpURLConnection) Operator_url.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(1000 * 10);
            conn.connect();
            int responseCode = conn.getResponseCode();
            if (responseCode == 200) {
                res = LDNetUtil.getStringFromStream(conn.getInputStream());
                if (conn != null) {
                    conn.disconnect();
                }
            }
            return res;
        } catch (MalformedURLException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } finally {
            if (conn != null) {
                conn.disconnect();
            }
        }
        return res;
    }

    /**
     * ping
     */
    @Override
    public void OnNetPingFinished(String log) {
        this.recordStepInfo(log);
    }

    @Override
    protected ThreadPoolExecutor getThreadPoolExecutor() {
        // TODO Auto-generated method stub
        return sExecutor;
    }

}
