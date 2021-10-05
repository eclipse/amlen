/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */
package svt.util;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.eclipse.paho.client.mqttv3.MqttMessage;


public class SVTStats {
    private Lock fmutex = null;
    private Lock nmutex = null;
    private Lock tmutex = null;
    private Lock mutex = null;
    private long starttime = 0;
    private long beforemessage = 0;
    private long aftermessage = 0;
    private long deficit = 0;
    private long lastupdate = 0;
    private long count = 0;
    private long icount = 0;
    private int rate = 0;
    private long topicCount;
    String note = null;
    long failed = 0;

    public SVTStats(int rate, int topicCount) {
        this.fmutex = new ReentrantLock();
        this.nmutex = new ReentrantLock();
        this.mutex = new ReentrantLock();
        this.tmutex = new ReentrantLock();
        this.starttime = System.currentTimeMillis();
        this.lastupdate = starttime;
        this.count = 0;
        this.rate = rate;
        this.topicCount = topicCount;
    }

    public long reinit() {
        long t = System.currentTimeMillis();
        mutex.lock();
        starttime = t;
        lastupdate = t;
        count = 0;
        mutex.unlock();
        return t;
    }

    public long increment() {
        long c = 0;
        long t = System.currentTimeMillis();
        lastupdate = t;
        mutex.lock();
        c = ++count;
        icount++;
        mutex.unlock();
        return (c);
    }

    public long getICount() {
        long c = 0;
        mutex.lock();
        c = icount;
        icount = 0;
        mutex.unlock();
        return (c);
    }

    public long getCount() {
        long c = 0;
        mutex.lock();
        c = count;
        mutex.unlock();
        return (c);
    }

    public void setNote(String note) {
        nmutex.lock();
        this.note = new String(note);
        nmutex.unlock();
    }

    public String getNote() {
        String n = null;
        nmutex.lock();
        n = this.note;
        this.note = null;
        nmutex.unlock();
        return (n);
    }

    public void setFailedCount(long failed) {
        fmutex.lock();
        this.failed = failed;
        fmutex.unlock();
    }

    public long getFailedCount() {
        long f = 0;
        fmutex.lock();
        f = this.failed;
        this.failed = 0;
        fmutex.unlock();
        return (f);
    }

    public double getRunTimeMin() {
        return getRunTimeSec() / 60.0;
    }

    public double getRunTimeSec() {
        return getRunTimeMS() / 1000.0;
    }

    public void setTopicCount(long topicCount) {
        tmutex.lock();
        this.topicCount = topicCount;
        tmutex.unlock();
    }

    public long getTopicCount() {
        long tc = 0;
        tmutex.lock();
        tc = this.topicCount;
        tmutex.unlock();
        return tc;
    }

    public long getRunTimeMS() {
        long d = 0;
        // mutex.lock();
        d = lastupdate - starttime;
        // mutex.unlock();
        return (d);
    }

    public long getDeltaRunTimeMS() {
        long d = 0;
        // mutex.lock();
        d = lastupdate - starttime;
        // mutex.unlock();
        return (d);
    }

    public long beforeMessage() {
        beforemessage = System.currentTimeMillis();
        return (beforemessage);
    }

    public long afterMessageRateControl() throws InterruptedException {
        aftermessage = System.currentTimeMillis();
        deficit += rate - (aftermessage - beforemessage);
        if (deficit > 0) {
            Thread.sleep(deficit);
            deficit = deficit - (System.currentTimeMillis() - aftermessage);
        }
        return (aftermessage);
    }

    public long getDeltaLoopRunTimeMS() {
        return (aftermessage - beforemessage);
    }

    public double getRatePerMin() {
        return getRatePerSec() * 60.0;
    }

    public MqttMessage getRateMessage(String clientId, int tlistsize) {
        String text = "Rate:" + clientId + ":" + tlistsize + ":" + getRatePerSec();
        MqttMessage rateMessage = new MqttMessage(text.getBytes());
        rateMessage.setQos(0);
        return rateMessage;
    }

    public double getRatePerSec() {
        return getRatePerMS() * 1000.0;
    }

    public double getRatePerMS() {
        double r = 0;
        // mutex.lock();
        if (lastupdate != starttime) {
            r = (double) (count) / (double) (lastupdate - starttime);
        }
        // mutex.unlock();
        return (r);
    }
}
