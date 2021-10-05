/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
/*-----------------------------------------------------------------
 * System Name : MQ Low Latency Messaging
 * File        : TokenBucket.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import java.util.Timer;
import java.util.TimerTask;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 * <p>
 */
public final class TokenBucket {

    // Rate (messages/second)
    private int            _rate;

    // Burst -- Total number of messages that can be in the bucket
    private int            _burst;

    // While _running is true, tokens will continue dropping into the bucket
    private boolean        _running;

    // The total number of tokens in the bucket
    private int            _numTokens;
  
    private Timer          _timer = null;   
    private final Object   _mutex;  
    private final long     _resetTimeout;
    private final TrWriter _trWriter;
    private final String   _id;    
    
    // Default constructor that creates and initializes a token bucket
    public TokenBucket(int tokenBucketRate, int tokenBucketBurst,TrWriter trWriter, String id){
        _mutex = new Object();
        synchronized (_mutex) {
            _running = false;
            _burst = tokenBucketBurst;
            _rate = tokenBucketRate;
            _numTokens = 0;
            _resetTimeout = (long) (_burst/(0.001*_rate))+1L;
            _trWriter = trWriter;
            _id = id;
        }
    }

    // Attempt to remove a token from the bucket.
    // This will return false if the bucket is empty.
    public boolean removeToken(long timeout){
        synchronized (_mutex) {
            long expirationTime = (timeout > 0) ? (System.currentTimeMillis() + timeout) : Long.MAX_VALUE;
            while((_numTokens == 0)&&(_running)){
                long waitTime = (expirationTime - System.currentTimeMillis());
                try {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD7304", 
                            "TokenBucket {0}: Going to wait. waitTime = {1}"
                            ,_id,waitTime); 
                    _mutex.wait(waitTime);
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD7304", 
                            "TokenBucket {0}: After wait.",_id); 
                } catch (InterruptedException e) {
                    _trWriter.writeException(e);
                }
            }
            if(_numTokens > 0){
                _numTokens--;
                return true;
            }else{
                return false;
            }
        }
    }

    // Start adding tokens to the bucket
    public void start(){
        synchronized (_mutex) {
            _running = true;
            _numTokens = _burst;
            TimerTask tt = new TimerTask(){
                @Override
                public void run() {
                    synchronized (_mutex) {            
                        if(!_running) {
                            return;                        
                        }
                        boolean wasEmpty = (_numTokens == 0);
                        _numTokens = _burst;
                        if(wasEmpty){
                            _mutex.notifyAll();
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMTD7305", 
                                    "TokenBucket {0}: An empty token bucket is refilled.",_id);
                        }
                    }
                }
            };
            _timer = new Timer("TokenBucketTimer."+_id, true);
            _timer.schedule(tt, _resetTimeout,_resetTimeout);
            _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD7302", 
                    "TokenBucket {0}:{1}:{2}:{3}: Started.", 
                    _id,_rate,_burst,_resetTimeout);
        }
    }

    // Stop adding tokens to the bucket
    public void stop(){
        synchronized (_mutex) {
            _running = false;
            _timer.cancel();
            _timer = null;
            _mutex.notifyAll();
            _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD7303", 
                    "TokenBucket {0}: Stopped.",_id); 
        }
    }


    public Object clone(){
        return new TokenBucket(_rate,_burst,_trWriter,_id);
    }
    
}
