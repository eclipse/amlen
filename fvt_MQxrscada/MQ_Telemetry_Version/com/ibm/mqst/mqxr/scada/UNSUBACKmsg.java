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

    byte[] msgIdentifier;

    // Parse constructor from superclass (see MsgTemplate.java)
    public UNSUBACKmsg(DataInputStream in) throws IOException
    {
        super(in);
    }

    public UNSUBACKmsg(DataInputStream in, boolean readHeader) throws IOException
    {
        super(in, readHeader ? MsgTemplate.getFixedHeader(in) : Headers.UNSUBACK);
    }

    // Basic constructor - takes msgId - rest of data is constant
    public UNSUBACKmsg(MsgIdentifier id)
    {

    	fixedHeader = SCADAutils.setBits(1, 0, 1, 1, 0, 0, 1, 0);
        remainingLength = new byte[1];
        remainingLength[0] = SCADAutils.setBits(0, 0, 0, 0, 0, 0, 1, 0);

        msgIdentifier = id.msgId;

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

    // Runs through and prints out message byte information
    public void parseAndPrint(LogFile log) throws IOException
    {
        super.parseAndPrint(log);
        int offset = 2;

        // Print Msg ID
        SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID(1)", offset);
        offset++;
        SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID(2)", offset);
        offset++;

    }

}
