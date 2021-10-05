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
/* This class represents SDP message type UNSUBSCRIBE */

public class UNSUBSCRIBEmsg extends MsgTemplate
{
	public static final int MYTYPE = 10;

    byte[] msgIdentifier;

    byte[] payload;

    TopicList myTopicList;

    MsgIdentifier myId;
    
    int expectedRc = 0;

    // From abstract class - reads in msg from DataInputStream
    public UNSUBSCRIBEmsg(DataInputStream in) throws IOException
    {
        super(MYTYPE, in);
    }

    public UNSUBSCRIBEmsg(DataInputStream in, byte header) throws IOException
    {
        super(MYTYPE, in, header);
    }

    // Basic constructor - takes a subList (see subList.java) containing topic
    // names followed by QOS level
    // and a msg id
    public UNSUBSCRIBEmsg(TopicList topics, MsgIdentifier id) throws ScadaException
    {
    	super(MYTYPE);

        fixedHeader = SCADAutils.setBits(1, 0, 1, 0, 0, 0, 1, 0);
        msgIdentifier = id.msgId;
        payload = topics.toByteArray();
        myId = id;
        myTopicList = topics;

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Works out the length of variable part of msg
    private int variableLength()
    {

        int result = 0;
        if (SCADAutils.version >= 5) 
            result++;
        result += msgIdentifier.length;
        result += payload.length;
        return result;
    }

    // Assembles full byte array
    private void buildMsg() throws ScadaException
    {

        int size = variableLength();
        ;
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
        if (SCADAutils.version >= 5) {
            construct.addElement(new Byte((byte)0));    
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

    // Use this function to send the msg and have the UNSUBACK
    // reponse handled automatically
    public void sendAndAcknowledge(DataInputStream input, DataOutputStream output, LogFile subLog, LogFile resultsLog) 
            throws IOException, ScadaException
    {

        send(output, subLog);

        resultsLog.println("Message sent - waiting for reply");

        UNSUBACKmsg unSubAck = new UNSUBACKmsg(input);
        UNSUBACKmsg unSubAckEX = new UNSUBACKmsg(myId, myTopicList.getReasons(expectedRc));

        // compare received and expected UNSUBACK, ignoring QoS bytes
        SCADAutils.compareAndLog(unSubAck, unSubAckEX, subLog, resultsLog, true);
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
                offset++;
            }
            
            // Print Topics
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

            }
    	}
    }

}
