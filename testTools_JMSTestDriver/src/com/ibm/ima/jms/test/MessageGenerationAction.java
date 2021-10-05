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

import java.io.File;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 */
public class MessageGenerationAction extends Action {

    private final File msgFile;
    private final int nummsgs;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public MessageGenerationAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        try {
            if (_actionParams.containsKey("filename")) {
                msgFile = new File(_actionParams.getProperty("filename"));
            } else {
                msgFile = null;
            }
            if (_actionParams.containsKey("nummsgs")) {
                nummsgs = Integer
                        .parseInt(_actionParams.getProperty("nummsgs"));
            } else {
                nummsgs = 0;
            }
            if (msgFile == null) {
                throw new RuntimeException(
                        "Filename is not defined for MessageGenerationAction:"
                                + _id);
            }
            if (nummsgs == 0) {
                throw new RuntimeException(
                        "Max number of messages is not defined for MessageGenerationAction:"
                                + _id);
            }
        } catch (Throwable t) {
            throw new RuntimeException(
                    "Failed to parse configuration parameters for action: "
                            + _id, t);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.mds.rm.test.Action#run()
     */
    @Override
    protected final boolean run() {
        boolean rc;
        try {
            rc = invokeApi();
        } catch (Throwable e) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2100",
                    "Action {0} failed. The reason is: {1}", _id,
                    e.getMessage());
            _trWriter.writeException(e);
            rc = false;
        }
        return rc;
    }

    protected boolean invokeApi() throws Exception {
        /*
         * RmmTxMessage obj = new RmmTxMessage(); try{ DataOutputStream
         * msgStream = (msgFile != null) ? new DataOutputStream(new
         * FileOutputStream(msgFile)) : null; for (int j = 0; j < nummsgs; j++)
         * { obj.msgOff = 0; synchronized (RANDOM) { obj.msgLen =
         * RANDOM.nextInt(maxlen); while(obj.msgLen <= 0) obj.msgLen =
         * RANDOM.nextInt(maxlen); } obj.msgBuf = new byte[obj.msgLen];
         * synchronized (RANDOM) { RANDOM.nextBytes(obj.msgBuf); } if(msgStream
         * != null){ msgStream.writeInt(obj.msgLen); msgStream.write(obj.msgBuf,
         * 0, obj.msgLen); } } if (msgStream != null) { msgStream.flush();
         * msgStream.close(); } } catch (IOException t) {
         * _trWriter.writeException(t); return false; } catch (RuntimeException
         * e) { return false; }
         */
        return true;
    }
}
