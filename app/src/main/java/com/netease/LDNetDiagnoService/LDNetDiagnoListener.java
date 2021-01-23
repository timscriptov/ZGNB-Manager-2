package com.netease.LDNetDiagnoService;

/**
 * @author panghui
 */
public interface LDNetDiagnoListener {

    /**
     * @param log
     */
    public void OnNetDiagnoFinished(String log);


    /**
     * @param log
     */
    public void OnNetDiagnoUpdated(String log);
}
