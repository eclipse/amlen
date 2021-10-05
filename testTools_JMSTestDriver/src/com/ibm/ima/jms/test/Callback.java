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
 * File        : Callback.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import java.util.Properties;
import java.util.Random;

/**
 * <p>
 * 
 */
public abstract class Callback {
    protected final TrWriter _trWriter;
    protected final String   _id;
    private final int        _exceptionChance;
    private final Random     _random;
    private final long       _delay;
    private final int        _delayChance;
    protected Callback(String id, Properties config, TrWriter trWriter) {
        _exceptionChance = Integer.parseInt(config.getProperty("exceptionChance","-1"));
        _delay = Long.parseLong(config.getProperty("delay","1"));
        _delayChance = Integer.parseInt(config.getProperty("delayChance","0"));
        if((_exceptionChance >= 0) || (_delayChance > 0))
            _random = new Random(System.currentTimeMillis());
        else
            _random = null;
        _trWriter = trWriter;
        _id = id;
    }
    //This method can be called from synchronized methods only
    protected final void callback() {
        if (_delayChance > 0) {
            int i = _random.nextInt(_delayChance);
            if (i == 0) {
                try {
                    wait(_delay);
                } catch (InterruptedException e) {
                    _trWriter.writeException(e);
                }
            }
            
        }
        if (_exceptionChance >= 0) {
            int i = _random.nextInt(_exceptionChance);
            if (i == 0) {
                throw new RuntimeException("Test exception was thrown by " + _id);
            }
        }        
        return;
    }
}
