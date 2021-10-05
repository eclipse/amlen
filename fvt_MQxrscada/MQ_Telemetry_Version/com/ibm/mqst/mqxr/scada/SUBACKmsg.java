package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.Vector;

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
/* This class represents SDP message type SUBACK */

public class SUBACKmsg extends MsgTemplate
{

    byte[] msgIdentifier;

    byte[] grantedQOS;

    // Parse constructor from superclass (see MsgTemplate.java)
    public SUBACKmsg(DataInputStream in) throws IOException
    {
        super(in);
    }

    public SUBACKmsg(DataInputStream in, boolean readHeader) throws IOException
    {
        super(in, readHeader ? MsgTemplate.getFixedHeader(in) : Headers.SUBACK);
    }

    // Default constructor (not used in client except for creating expected
    // msgs)
    public SUBACKmsg(SubList topics, MsgIdentifier id) throws ScadaException
    {

    	fixedHeader = SCADAutils.setBits(1, 0, 0, 1, 0, 0, 1, 0); // Note that
                                                                    // Qos1 is
                                                                    // specified
                                                                    // - even
                                                                    // thought
                                                                    // this byte
                                                                    // is unued,
                                                                    // it gets
                                                                    // set to 1
                                                                    // by the
                                                                    // broker so
                                                                    // this is
                                                                    // needed
                                                                    // for
                                                                    // comparisons
                                                                    // to pass
        msgIdentifier = id.msgId;
        grantedQOS = topics.qosArray();

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Works out the length of variable part of msg
    private int variableLength()
    {

        int result = 0;

        result += msgIdentifier.length;
        result += grantedQOS.length;
        return result;
    }

    // Assembles full byte array
    private void buildMsg() throws ScadaException
    {

        int size = variableLength();
        size += 1; // Fixed Header
        size += remainingLength.length;

        completeMsg = new byte[size];
        Vector construct = new Vector();

        // Build up message in constructor Vector then assign to byte array

        construct.addElement(new Byte(fixedHeader));

        for (int i = 0; i < remainingLength.length; i++)
        {
            construct.addElement(new Byte(remainingLength[i]));
        }

        for (int i = 0; i < msgIdentifier.length; i++)
        {
            construct.addElement(new Byte(msgIdentifier[i]));
        }

        for (int i = 0; i < grantedQOS.length; i++)
        {
            construct.addElement(new Byte(grantedQOS[i]));
        }

        if (construct.size() != size)
        {
            ScadaException myE = new ScadaException("Exception: Size mismatch when constructing msg in function SUBSCRIBEmsg buildMsg()");
            throw myE;
        }
        else
        {
            for (int i = 0; i < size; i++)
            {
                completeMsg[i] = ((Byte) (construct.elementAt(i))).byteValue();
            }
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

        Vector restOfMsg = new Vector();

        for (; offset < completeMsg.length; offset++)
        {
            restOfMsg.addElement(new Byte(completeMsg[offset]));
        }

        grantedQOS = new byte[restOfMsg.size()];

        for (int i = 0; i < grantedQOS.length; i++)
        {
            grantedQOS[i] = ((Byte) (restOfMsg.elementAt(i))).byteValue();
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

        // Print Granted QoS
        while (offset < completeMsg.length)
        {
            SCADAutils.printByteNumeric(completeMsg[offset], log, "GRANTED QOS", offset);
            offset++;
        }
    }

}
