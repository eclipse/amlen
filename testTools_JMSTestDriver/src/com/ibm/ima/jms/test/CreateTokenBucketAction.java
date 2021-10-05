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
/**
 * 
 */
package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

/**
 *
 */
public class CreateTokenBucketAction extends Action {    
    private final String      _structureID;
    private final TokenBucket _bucket;
    
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateTokenBucketAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        int rate = Integer.parseInt(_actionParams.getProperty("rate","1000"));
        int burst = Integer.parseInt(_actionParams.getProperty("rate","100"));
        _bucket = new TokenBucket(rate, burst, trWriter, _structureID);
    }
    
    /* (non-Javadoc)
     * @see com.ibm.mds.rm.test.Action#run()
     */
    @Override    
    protected final boolean run() {
        _bucket.start();
        _dataRepository.storeVar(_structureID, _bucket);
        return true;
    }


}
