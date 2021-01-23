package com.netease.LDNetDiagnoService;

import org.jetbrains.annotations.NotNull;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class LDNetTraceRoute {
    private static final String MATCH_TRACE_IP = "(?<=From )(?:[0-9]{1,3}\\.){3}[0-9]{1,3}";
    private static final String MATCH_PING_IP = "(?<=from ).*(?=: icmp_seq=1 ttl=)";
    private static final String MATCH_PING_TIME = "(?<=time=).*?ms";
    private static LDNetTraceRoute instance;
    private final String LOG_TAG = "LDNetTraceRoute";
    public boolean isCTrace = true;
    LDNetTraceRouteListener listener;

    private LDNetTraceRoute() {
    }

    public static LDNetTraceRoute getInstance() {
        if (instance == null) {
            instance = new LDNetTraceRoute();
        }
        return instance;
    }

    public void initListenter(LDNetTraceRouteListener listener) {
        this.listener = listener;
    }

    /**
     * hosttraceroute
     *
     * @param host
     * @return
     */
    public void startTraceRoute(String host) {
        if (isCTrace) {
            try {
                startJNICTraceRoute(host);
            } catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                TraceTask trace = new TraceTask(host, 1);
                execTrace(trace);
            }
        } else {
            TraceTask trace = new TraceTask(host, 1);
            execTrace(trace);
        }
    }

    public void resetInstance() {
        if (instance != null) {
            instance = null;
        }
    }

    /**
     * jni ctraceroute
     */
    public native void startJNICTraceRoute(String traceCommand);

    /**
     * jni c
     *
     * @param log
     */
    public void printTraceInfo(String log) {
        // Log.i(LOG_TAG, log);
        listener.OnNetTraceUpdated(log);
    }

    /**
     * ping，ping
     *
     * @param ping
     * @return
     */
    private String execPing(@NotNull PingTask ping) {
        Process process = null;
        String str = "";
        BufferedReader reader = null;
        try {
            process = Runtime.getRuntime().exec("ping -c 1 " + ping.getHost());
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
        return str;
    }

    /**
     * pingtraceroute
     *
     * @param trace
     * @return
     */
    private void execTrace(TraceTask trace) {
        Pattern patternTrace = Pattern.compile(MATCH_TRACE_IP);
        Pattern patternIp = Pattern.compile(MATCH_PING_IP);
        Pattern patternTime = Pattern.compile(MATCH_PING_TIME);

        Process process = null;
        BufferedReader reader = null;
        boolean finish = false;
        try {

            while (!finish && trace.getHop() < 30) {

                String str = "";

                String command = "ping -c 1 -t " + trace.getHop() + " "
                        + trace.getHost();

                process = Runtime.getRuntime().exec(command);
                reader = new BufferedReader(new InputStreamReader(
                        process.getInputStream()));
                String line = null;
                while ((line = reader.readLine()) != null) {
                    str += line;
                }
                reader.close();
                process.waitFor();

                Matcher m = patternTrace.matcher(str);


                StringBuilder log = new StringBuilder(256);
                if (m.find()) {
                    String pingIp = m.group();
                    PingTask pingTask = new PingTask(pingIp);

                    String status = execPing(pingTask);
                    if (status.length() == 0) {
                        log.append("unknown host or network error\n");
                        finish = true;
                    } else {
                        Matcher matcherTime = patternTime.matcher(status);
                        if (matcherTime.find()) {
                            String time = matcherTime.group();
                            log.append(trace.getHop());
                            log.append("\t\t");
                            log.append(pingIp);
                            log.append("\t\t");
                            log.append(time);
                            log.append("\t");
                        } else {
                            log.append(trace.getHop());
                            log.append("\t\t");
                            log.append(pingIp);
                            log.append("\t\t timeout \t");
                        }
                        listener.OnNetTraceUpdated(log.toString());
                        trace.setHop(trace.getHop() + 1);
                    }
                } else {
                    Matcher matchPingIp = patternIp.matcher(str);
                    if (matchPingIp.find()) {
                        String pingIp = matchPingIp.group();
                        Matcher matcherTime = patternTime.matcher(str);
                        if (matcherTime.find()) {
                            String time = matcherTime.group();
                            log.append(trace.getHop());
                            log.append("\t\t");
                            log.append(pingIp);
                            log.append("\t\t");
                            log.append(time);
                            log.append("\t");
                            listener.OnNetTraceUpdated(log.toString());
                        }
                        finish = true;
                    } else {
                        if (str.length() == 0) {
                            log.append("unknown host or network error\t");
                            finish = true;
                        } else {
                            log.append(trace.getHop());
                            log.append("\t\t timeout \t");
                            trace.setHop(trace.getHop() + 1);
                        }
                        listener.OnNetTraceUpdated(log.toString());
                    }
                }// else no match traceIPPattern
            }// while
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

        listener.OnNetTraceFinished();
    }

    /**
     * NetPingService
     *
     * @author panghui
     */
    public interface LDNetTraceRouteListener {
        public void OnNetTraceUpdated(String log);

        public void OnNetTraceFinished();
    }

    /**
     * Ping
     *
     * @author panghui
     */
    private class PingTask {

        private static final String MATCH_PING_HOST_IP = "(?<=\\().*?(?=\\))";
        private String host;

        public PingTask(String host) {
            super();
            this.host = host;
            Pattern p = Pattern.compile(MATCH_PING_HOST_IP);
            Matcher m = p.matcher(host);
            if (m.find()) {
                this.host = m.group();
            }

        }

        public String getHost() {
            return host;
        }
    }

    /**
     * trace
     *
     * @author panghui
     */
    private class TraceTask {
        private final String host;
        private int hop;

        public TraceTask(String host, int hop) {
            super();
            this.host = host;
            this.hop = hop;
        }

        public String getHost() {
            return host;
        }

        public int getHop() {
            return hop;
        }

        public void setHop(int hop) {
            this.hop = hop;
        }
    }
}
