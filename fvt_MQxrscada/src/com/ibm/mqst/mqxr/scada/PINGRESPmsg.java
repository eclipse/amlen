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
/* This class represents SDP message type PINGRESP */

public class PINGRESPmsg extends MsgTemplate
{
	public static final int MYTYPE = 13;

    // Parse constructor from superclass (see MsgTemplate.java)
    public PINGRESPmsg(DataInputStream in) throws IOException
    {
        super(MYTYPE, in);
    }

    public PINGRESPmsg(DataInputStream in, byte header) throws IOException
    {
        super(MYTYPE, in, header);
    }

    // Basic constructor - takes msgId - rest of data is constant (not used in
    // client except for creating expected msgs)
    public PINGRESPmsg()
    {
    	super(MYTYPE);

        fixedHeader = SCADAutils.setBits(1, 1, 0, 1, 0, 0, 0, 0);
        remainingLength = new byte[1];
        remainingLength[0] = SCADAutils.setBits(0, 0, 0, 0, 0, 0, 0, 0);

        buildMsg();
    }

    // Assembles full byte array
    private void buildMsg()
    {

        int size = 2;

        completeMsg = new byte[size];

        completeMsg[0] = fixedHeader;
        completeMsg[1] = remainingLength[0];

    }

    // Sets all variable fields using data read into complete msg
    // - checks it can be parsed after being read in from an Input
    // Stream
    protected void setFields()
    {

        // Does nothing - fixed header and variable length set already

    }

}
