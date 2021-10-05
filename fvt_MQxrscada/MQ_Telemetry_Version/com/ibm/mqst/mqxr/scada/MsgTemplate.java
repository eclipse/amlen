package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
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
/*
 * This class is used as a base for all SDP message types and contains common
 * functions and data
 */

public abstract class MsgTemplate
{

    byte fixedHeader;

    byte[] remainingLength;

    byte[] completeMsg;

    // Default constructor
    public MsgTemplate()
    {
    }


    // Parse constructor - builds a connect msg from an input stream
    public MsgTemplate(DataInputStream in) throws IOException
    {
        this(in, getFixedHeader(in));
    }
    
    // Parse constructor - builds a connect msg from an input stream
    public MsgTemplate(DataInputStream in, byte fixedHeader) throws IOException
    {
        this.fixedHeader = fixedHeader;
        
        remainingLength = SCADAutils.setRemainingLength(in);

        completeMsg = new byte[1 + remainingLength.length + SCADAutils.remainingLengthAsDecimal(remainingLength)];

        completeMsg[0] = fixedHeader;

        for (int i = 0; i < remainingLength.length; i++)
        {
            completeMsg[i + 1] = remainingLength[i];
        }

        for (int i = 1 + remainingLength.length; i < completeMsg.length; i++)
        {
            completeMsg[i] = in.readByte();
        }

        setFields();
    }
    
    protected static byte getFixedHeader(DataInputStream in) throws IOException
    {
        byte fixedHeader;
        try
        {
            fixedHeader = in.readByte();
        }
        catch (EOFException e)
        {
            // ignore first EOFException, sleep for 10 seconds and try again- message may not have arrived yet
            SCADAutils.sleep(10000);
            fixedHeader = in.readByte();
        }
        
        return fixedHeader;
    }

    // Sets all variable fields using data read into complete msg
    // - checks it can be parsed after being read in from an Input
    // Stream
    protected abstract void setFields() throws IOException;

    // Sends the msg on the stream provided
    public void send(DataOutputStream out, LogFile log) throws IOException
    {
            log.printTitle(SCADAutils.getMsgType(fixedHeader) + " msg sent to broker");
            parseAndPrint(log);
            out.write(completeMsg);
            out.flush();
    }

    // Runs through the message printing out byte information
    public void parseAndPrint(LogFile log) throws IOException
    {
        log.println("");
        log.print("BYTE[0] =\t");
        SCADAutils.printBinary(fixedHeader, log);
        log.print("\t\tHEADER: ");
        log.print("||Msg Type: " + SCADAutils.getMsgType(fixedHeader));
        log.print("||DUP: " + SCADAutils.isDupSet(fixedHeader));
        log.print("||QoS: " + SCADAutils.getQosAsString(fixedHeader));
        log.print("||RETAIN: " + SCADAutils.isRetainSet(fixedHeader));
        log.newLine();

        for (int i = 0; i < remainingLength.length; i++)
        {
            SCADAutils.printByteNumeric(remainingLength[i], log, "REMAINING LENGTH", (i + 1));
        }
    }

}
