package com.netease.LDNetDiagnoService;

import android.util.Log;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.List;

public class LDNetSocket {
    private static final int PORT = 80;
    private static final int CONN_TIMES = 4;
    private static final String TIMEOUT = "DNS resolution is normal, connection timeout, TCP establishment failure";
    private static final String IOERR = "DNS resolution is normal, IO is abnormal, TCP establishment failed";
    private static final String HOSTERR = "DNS resolution failed, host address is unreachable";
    static boolean loaded;
    private static LDNetSocket instance = null;

    static {
        try {
            System.loadLibrary("tracepath");
            loaded = true;
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private final long[] RttTimes = new long[CONN_TIMES];
    public InetAddress[] _remoteInet;
    public List<String> _remoteIpList;
    public boolean isCConn = true;
    private LDNetSocketListener listener;
    private int timeOut = 6000;
    private boolean[] isConnnected;

    private LDNetSocket() {

    }

    public static LDNetSocket getInstance() {
        if (instance == null) {
            instance = new LDNetSocket();
        }
        return instance;
    }

    public void initListener(LDNetSocketListener listener) {
        this.listener = listener;
    }

    /**
     * connectTCPRTT
     */
    public boolean exec(String host) {
        if (isCConn && loaded) {
            try {
                startJNITelnet(host, "80");
                return true;
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                Log.i("LDNetSocket", "call jni failed, call execUseJava");
                return execUseJava(host);
            }
        } else {
            return execUseJava(host);
        }
    }

    /**
     * javaconnected
     */
    private boolean execUseJava(String host) {
        if (_remoteInet != null && _remoteIpList != null) {
            int len = _remoteInet.length;
            isConnnected = new boolean[len];
            for (int i = 0; i < len; i++) {
                if (i != 0) {
                    this.listener.OnNetSocketUpdated("\n");
                }
                isConnnected[i] = execIP(_remoteInet[i], _remoteIpList.get(i));
            }
            for (Boolean i : isConnnected) {
                if (i == true) {
                    this.listener.OnNetSocketFinished("\n");
                    return true;
                }
            }

        } else {
            this.listener.OnNetSocketFinished(HOSTERR);
        }
        this.listener.OnNetSocketFinished("\n");
        return false;
    }

    /**
     * IP5connect
     */
    private boolean execIP(InetAddress inetAddress, String ip) {
        boolean isConnected = true;
        StringBuilder log = new StringBuilder();
        InetSocketAddress socketAddress = null;
        if (inetAddress != null && ip != null) {
            socketAddress = new InetSocketAddress(inetAddress, PORT);
            int flag = 0;
            this.listener.OnNetSocketUpdated("Connect to host: " + ip + "..." + "\n");
            for (int i = 0; i < CONN_TIMES; i++) {
                execSocket(socketAddress, timeOut, i);
                if (RttTimes[i] == -1) {
                    this.listener.OnNetSocketUpdated((i + 1) + "'s time=" + "TimeOut"
                            + ",  ");
                    timeOut += 4000;
                    if (i > 0 && RttTimes[i - 1] == -1) {
                        flag = -1;
                        break;
                    }
                } else if (RttTimes[i] == -2) {
                    this.listener
                            .OnNetSocketUpdated((i + 1) + "'s time=" + "IOException");
                    if (i > 0 && RttTimes[i - 1] == -2) {
                        flag = -2;
                        break;
                    }
                } else {
                    this.listener.OnNetSocketUpdated((i + 1) + "'s time=" + RttTimes[i]
                            + "ms,  ");
                }
            }
            long time = 0;
            int count = 0;
            if (flag == -1) {
                // log.append(TIMEOUT);
                isConnected = false;
            } else if (flag == -2) {
                // log.append(IOERR);
                isConnected = false;
            } else {
                for (int i = 0; i < CONN_TIMES; i++) {
                    if (RttTimes[i] > 0) {
                        time += RttTimes[i];
                        count++;
                    }
                }
                if (count > 0) {
                    time = time / count;
                    log.append("average=" + time + "ms");
                }
            }
        } else {
            isConnected = false;
        }
        this.listener.OnNetSocketUpdated(log.toString());
        return isConnected;
    }

    /**
     * IPindexconnect
     */
    private void execSocket(InetSocketAddress socketAddress, int timeOut,
                            int index) {
        Socket socket = null;
        long start = 0;
        long end = 0;
        try {
            socket = new Socket();
            start = System.currentTimeMillis();
            socket.connect(socketAddress, timeOut);
            end = System.currentTimeMillis();
            RttTimes[index] = end - start;
        } catch (SocketTimeoutException e) {
            RttTimes[index] = -1;
            e.printStackTrace();
        } catch (IOException e) {
            RttTimes[index] = -2;
            e.printStackTrace();
        } finally {
            if (socket != null) {
                try {
                    socket.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void resetInstance() {
        if (instance != null) {
            instance = null;
        }
    }

    /*
     * jninative
     */
    public native void startJNITelnet(String host, String port);

    public void printSocketInfo(String log) {
        listener.OnNetSocketUpdated(log);
    }

    public interface LDNetSocketListener {
        public void OnNetSocketFinished(String log);

        public void OnNetSocketUpdated(String log);
    }

}
