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
/* This class represents SDP message type UNSUBACK */

public class UNSUBACKmsg extends MsgTemplate
{
	public static final int MYTYPE = 11;

    byte[] msgIdentifier;
    byte[] reason;

    // Parse constructor from superclass (see MsgTemplate.java)
    public UNSUBACKmsg(DataInputStream in) throws IOException
    {
        super(MYTYPE, in);
    }

    public UNSUBACKmsg(DataInputStream in, byte header) throws IOException
    {
        super(MYTYPE, in, header);
    }

    // Basic constructor - takes msgId - rest of data is constant
    public UNSUBACKmsg(MsgIdentifier id)
    {
    	super(MYTYPE);

    	fixedHeader = SCADAutils.setBits(1, 0, 1, 1, 0, 0, 1, 0);
        remainingLength = new byte[1];
        remainingLength[0] = 2;
        if (SCADAutils.version >= 5) {
            remainingLength[0] = 4;
        }
        reason = new byte[1];
        reason[0] = 0;
        msgIdentifier = id.msgId;

        buildMsg();
    }
    
    // Basic constructor - takes msgId - rest of data is constant
    public UNSUBACKmsg(MsgIdentifier id, byte [] rc)
    {
        super(MYTYPE);

        fixedHeader = SCADAutils.setBits(1, 0, 1, 1, 0, 0, 0, 0);
        remainingLength = new byte[1];
        remainingLength[0] = 2;
        if (SCADAutils.version >= 5) {
            remainingLength[0] = (byte)(3 + rc.length);
        }
        reason = rc;
        msgIdentifier = id.msgId;

        buildMsg();
    }

    // Assembles full byte array
    private void buildMsg()
    {

        int size = 2; // MsgId
        size += 1; // Fixed Header
        size += 1; // Remaining Length
        if (SCADAutils.version >= 5) {
            size++;
            size += reason.length;
        }    

        completeMsg = new byte[size];

        completeMsg[0] = fixedHeader;
        completeMsg[1] = remainingLength[0];
        completeMsg[2] = msgIdentifier[0];
        completeMsg[3] = msgIdentifier[1];
        if (SCADAutils.version >= 5) {
            completeMsg[4] = 0;   /* props */
            System.arraycopy(reason, 0, completeMsg, 5, reason.length);
        }
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
        if (SCADAutils.version >= 5) {
            offset++;   /* Assume 0 props */
            if (offset < completeMsg.length) {
                reason = new byte[completeMsg.length-offset];
                System.arraycopy(completeMsg, offset, reason, 0, reason.length);
            }
        }

    }

    // Runs through and prints out message byte information
    public void parseAndPrint(LogFile log) throws IOException
    {
    	if (null != realMsg) {
    		realMsg.parseAndPrint(log);
    	} else {
    		super.parseAndPrint(log);
    		int offset = 2;

    		// Print Msg ID
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID(1)", offset);
    		offset++;
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID(2)", offset);
    		offset++;
    		if (SCADAutils.version >= 5) {
    		    while (offset < completeMsg.length) {
    		        SCADAutils.printByteNumeric(completeMsg[offset], log, "REASON", offset);
    		        offset++;
    		    }
    		}
    	}

    }

}
