package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.DataOutputStream;
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
/* This class represents SDP message type SUBSCRIBE */

public class SUBSCRIBEmsg extends MsgTemplate
{

    byte[] msgIdentifier;

    byte[] payload;

    SubList mySubList;

    MsgIdentifier myId;

    // From abstract class - reads in msg from DataInputStream
    public SUBSCRIBEmsg(DataInputStream in) throws IOException
    {
        super(in);
    }

    public SUBSCRIBEmsg(DataInputStream in, boolean readHeader) throws IOException
    {
        super(in, readHeader ? MsgTemplate.getFixedHeader(in) : Headers.SUBSCRIBE);
    }

    // Basic constructor - takes a subList (see subList.java) containing topic
    // names followed by QOS level
    // and a msg id
    public SUBSCRIBEmsg(SubList topics, MsgIdentifier id) throws ScadaException
    {
        this(topics, id, 1);
    }

    // Basic constructor - takes a subList (see subList.java) containing topic
    // names followed by QOS level
    // and a msg id
    public SUBSCRIBEmsg(SubList topics, MsgIdentifier id, int qosLevel) throws ScadaException
    {
        switch (qosLevel)
        {
            case 0 : fixedHeader = (byte) 0x80; break;
            case 1 : fixedHeader = SCADAutils.fixedHeaderSUBSCRIBE; break;
            case 2 : fixedHeader = (byte) 0x86; break;
        }
        
        fixedHeader = SCADAutils.fixedHeaderSUBSCRIBE;
        msgIdentifier = id.msgId;
        myId = id;
        payload = topics.toByteArray();
        mySubList = topics;

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
        result += payload.length;
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

        for (int i = 0; i < payload.length; i++)
        {
            construct.addElement(new Byte(payload[i]));
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

        msgIdentifier[0] = completeMsg[offset];
        offset++;
        msgIdentifier[1] = completeMsg[offset];
        offset++;

        Vector restOfMsg = new Vector();

        for (; offset < completeMsg.length; offset++)
        {
            restOfMsg.addElement(new Byte(completeMsg[offset]));
        }

        payload = new byte[restOfMsg.size()];

        for (int i = 0; i < payload.length; i++)
        {
            payload[i] = ((Byte) (restOfMsg.elementAt(i))).byteValue();
        }

    }

    // Use this function to send a SUBSCRIBE msg and have the
    // responding SUBACK handled automatically - if the SUBACK
    // is not what is expected an exception is thrown

    public void sendAndAcknowledge(DataInputStream input, DataOutputStream output, LogFile subLog, LogFile resultsLog) throws IOException, ScadaException
    {

        send(output, subLog);

        resultsLog.println("Message sent - waiting for reply");

        SUBACKmsg subAck = new SUBACKmsg(input);
        SUBACKmsg subAckEX = new SUBACKmsg(mySubList, myId);

        // compare received and expected SUBACK, ignoring QoS bytes
        SCADAutils.compareAndLog(subAck, subAckEX, subLog, resultsLog, true);

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

        // Print Topics and required QoS
        while (offset < completeMsg.length)
        {

            // Topic name length

            byte[] topicNameLength = new byte[2];
            SCADAutils.printByteNumeric(completeMsg[offset], log, "TOPIC NAME LENGTH(1)", offset);
            topicNameLength[0] = completeMsg[offset];
            offset++;
            SCADAutils.printByteNumeric(completeMsg[offset], log, "TOPIC NAME LENGTH(2)", offset);
            topicNameLength[0] = completeMsg[offset];
            offset++;

            // Topic Name

            int topicLength = SCADAutils.remainingLengthAsDecimal(topicNameLength);

            for (int i = 0; i < topicLength; i++)
            {
                SCADAutils.printByteChar(completeMsg[offset], log, "TOPIC NAME", offset);
                offset++;
            }

            // Requested QoS

            SCADAutils.printByteNumeric(completeMsg[offset], log, "REQUESTED QOS", offset);
            offset++;

        }
    }

}
