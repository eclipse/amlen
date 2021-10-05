package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.IOException;

/*
 * Copyright (c) 2002-2021 Contributors to the Eclipse Foundation
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
 * @version 1.0
 */

/* This class represents SDP message type CONNACK */

public class CONNACKmsg extends MsgTemplate
{
	public static final int MYTYPE = 2;

    byte unused;

    byte returnCode;

    // Parse constructor from superclass (see MsgTemplate.java)
    public CONNACKmsg(DataInputStream in) throws IOException
    {
        super(MYTYPE, in);
    }

    public CONNACKmsg(DataInputStream in, byte header) throws IOException
    {
        super(MYTYPE, in, header);
    }

    // Default constructor (not used in client except for creating expected
    // msgs)
    public CONNACKmsg()
    {
        this((byte) 0);
    }
    
    public CONNACKmsg(byte returnCode)
    {
    	super(MYTYPE);
        fixedHeader = SCADAutils.setBits(0, 0, 1, 0, 0, 0, 0, 0);
        remainingLength = new byte[1];

        remainingLength[0] = SCADAutils.setBits(0, 0, 0, 0, 0, 0, 1, 0);

        unused = 0;
        this.returnCode = returnCode;

        buildMsg();
    }

    // Contructs the message as a byte array
    public void buildMsg()
    {

        completeMsg = new byte[4];

        completeMsg[0] = fixedHeader;
        completeMsg[1] = remainingLength[0];
        completeMsg[2] = unused;
        completeMsg[3] = returnCode;
    }

    // Sets all variable fields using data read into complete msg
    // - checks it can be parsed after being read in from an Input
    // Stream
    protected void setFields()
    {

        int offset = 1 + remainingLength.length;

        unused = completeMsg[offset];
        offset++;
        returnCode = completeMsg[offset];
    }

    // Runs through and prints out message byte information
    public void parseAndPrint(LogFile log) throws IOException
    {
    	if (null != realMsg) {
    		realMsg.parseAndPrint(log);
    	} else {
            super.parseAndPrint(log);
            int offset = 2;
            SCADAutils.printByteNumeric(completeMsg[offset], log, "PRESENT", offset);
            offset++;
            SCADAutils.printByteNumeric(completeMsg[offset], log, "RETURN CODE - " + SCADAutils.connectReply(returnCode), offset);
            offset++;
            while (offset < completeMsg.length) {
                SCADAutils.printByteNumeric(completeMsg[offset], log, "PROPERTIES", offset);
                offset++;
            }
            
    	}
    }
    
    public void setSessionPresent(boolean value) {
    	if (value && SCADAutils.version >= 4) {
    		unused = 1;
    	} else {
    		unused = 0;
    	}
		completeMsg[2] = unused;
    }

}
