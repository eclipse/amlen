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
/* This class represents SDP message type PUBREL */

public class PUBRELmsg extends MsgTemplate
{
	public static final int MYTYPE = 6;

    byte[] msgIdentifier;

    // Parse constructor from superclass (see MsgTemplate.java)
    public PUBRELmsg(DataInputStream in) throws IOException
    {
        super(MYTYPE, in);
    }

    public PUBRELmsg(DataInputStream in, byte header) throws IOException
    {
        super(MYTYPE, in, header);
    }

    // Basic constructor - takes msgId - rest of data is constant
    // MQTT V3.1 specification states that PUBRel is always QoS=1
    public PUBRELmsg(MsgIdentifier id)
    {
    	super(MYTYPE);
        
        fixedHeader = SCADAutils.setBits(0, 1, 1, 0, 0, 0, 1, 0);
        remainingLength = new byte[1];
        remainingLength[0] = SCADAutils.setBits(0, 0, 0, 0, 0, 0, 1, 0);

        msgIdentifier = id.msgId;

        buildMsg();
    }
    
    public void setDUP(boolean value)
    {
    	fixedHeader = SCADAutils.setBits(0, 1, 1, 0, value ? 1 : 0, 0, 1, 0);
    	buildMsg();
    }
    
    // Assembles full byte array
    private void buildMsg()
    {

        int size = 2; // MsgId
        size += 1; // Fixed Header
        size += 1; // Remaining Length

        completeMsg = new byte[size];

        completeMsg[0] = fixedHeader;
        completeMsg[1] = remainingLength[0];
        completeMsg[2] = msgIdentifier[0];
        completeMsg[3] = msgIdentifier[1];

    }

    // Sets all variable fields using data read into complete msg
    // - checks it can be parsed after being read in from an Input
    // Stream
    protected void setFields()
    {

        int offset = 1 + remainingLength.length;

        msgIdentifier = new byte[2];

        for (int i = 0; i < msgIdentifier.length; i++)
        {
            msgIdentifier[i] = completeMsg[offset];
            offset++;
        }

    }

    // Run through and prints out message byte information
    public void parseAndPrint(LogFile log) throws IOException
    {
    	if (null != realMsg) {
    		realMsg.parseAndPrint(log);
    	} else {
    		super.parseAndPrint(log);

    		// Print Msg Id
    		int offset = 2;
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID", offset);
    		offset++;
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID", offset);
    		offset++;
    	}
    }

}
