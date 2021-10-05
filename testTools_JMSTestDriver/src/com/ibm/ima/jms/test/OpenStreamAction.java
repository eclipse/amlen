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

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;

import org.w3c.dom.Element;

/**
 *
 */
public class OpenStreamAction extends Action {    
    enum StreamType {
        OBJECT, DATA
    }
    enum StreamMode {
        READ, WRITE
    }
    private final String     _structureID;
    private final String     _fileName;
    private final StreamMode _mode;
    private final StreamType _type;
    
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
     public OpenStreamAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _fileName = _actionParams.getProperty("name");
        if (_fileName == null) {
            throw new RuntimeException("File name is not defined for "
                    + this.toString());
        }
        _mode = Enum.valueOf(StreamMode.class, _actionParams.getProperty("mode","READ").toUpperCase());
        _type = Enum.valueOf(StreamType.class, _actionParams.getProperty("type","OBJECT").toUpperCase());
    }
    
    /* (non-Javadoc)
     * @see com.ibm.mds.rm.test.Action#run()
     */
    @Override    
    protected final boolean run() {
        File file = new File(_fileName);
        final Object stream;
        try {
            if(_mode == StreamMode.WRITE){
                final OutputStream os;
                if(_type == StreamType.DATA){
                    os = new DataOutputStream(new FileOutputStream(file));
                }else {
                    os = new ObjectOutputStream(new FileOutputStream(file));
                }
                stream = new TestOutputStream(os);                
            }else {
                final InputStream is;
                if(_type == StreamType.DATA){
                    is = new DataInputStream(new FileInputStream(file));
                }else {
                    is = new ObjectInputStream(new FileInputStream(file));
                }
                stream = new TestInputStream(is);                
            }            
        } catch (Throwable e) {
            _trWriter.writeException(e);
            return false;
        }
        _dataRepository.storeVar(_structureID, stream);
        return true;
    }


}
