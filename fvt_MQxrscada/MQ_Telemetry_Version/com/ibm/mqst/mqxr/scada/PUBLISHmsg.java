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
/* This class represents SDP message type PUBLISH */

public class PUBLISHmsg extends MsgTemplate
{

    byte[] topic;

    byte[] msgId;

    byte[] payload;

    int myQos;

    MsgIdentifier myId;

    // Used for SDP_PROTOCOL_NETWORK test. Allows a publish to be sent without a payload, ie: header only
    public PUBLISHmsg() throws ScadaException
    {
        fixedHeader = SCADAutils.setBits(0, 0, 1, 1, 0, 0, 0, 0);
        topic = new byte[0];
        payload = new byte[0];
        myQos = 0;

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(10);

        // Build complete byte array
        buildMsg();
    }
    
    // From abstract class - reads in msg from DataInputStream
    public PUBLISHmsg(DataInputStream in) throws IOException
    {
        super(in);
    }

    public PUBLISHmsg(DataInputStream in, boolean readHeader) throws IOException
    {
        super(in, readHeader ? MsgTemplate.getFixedHeader(in) : Headers.PUBLISH);
    }

    // Basic constructor - QoS 0 - takes topic name and payload
    public PUBLISHmsg(String top, byte[] data) throws ScadaException
    {

        fixedHeader = SCADAutils.setBits(0, 0, 1, 1, 0, 0, 0, 0);
        topic = SCADAutils.StringToUTF(top);
        payload = data;
        myQos = 0;

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Advanced constructor - QoS 1 or 2 - takes topic name , msg ID, payload
    // and Qos level
    public PUBLISHmsg(String top, MsgIdentifier id, byte[] data, short qos) throws ScadaException
    {

        // Set header to default
        fixedHeader = SCADAutils.setBits(0, 0, 1, 1, 0, 0, 0, 0);

        // Now try to set supplied qos - will throw exception if not valid
        fixedHeader = SCADAutils.setQos(fixedHeader, qos);
        myQos = qos;

        topic = SCADAutils.StringToUTF(top);
        msgId = id.msgId;
        myId = id;
        payload = data;

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Use this constructor to get a PUBLISH message off
    // the input stream AND handle the necessary responses
    // for Qos1 and Qos2 published msgs

    public PUBLISHmsg(DataInputStream input, DataOutputStream output, LogFile pubLog, LogFile resultsLog, PUBLISHmsg pubInEX) throws IOException, ScadaException
    {
        this(input, output, pubLog, resultsLog, pubInEX, false);
    }

    public PUBLISHmsg(DataInputStream input, DataOutputStream output, LogFile pubLog, LogFile resultsLog, PUBLISHmsg pubInEX, boolean clearInputStream) throws IOException, ScadaException
    {

        // Get the expected PUBLISH msg as normal
        this(input);

        if (myQos == 1)
        {
            // The msgId can be anything - so set the expected msgs ID to match:
            pubInEX.setId(this.myId);

            SCADAutils.compareAndLog(this, pubInEX, pubLog, resultsLog);

            acknowledgeQOS1(input, output, pubLog, resultsLog, clearInputStream);
        }
        else if (myQos == 2)
        {
            // The msgId can be anything - so set the expected msgs ID to match:
            pubInEX.setId(this.myId);

            SCADAutils.compareAndLog(this, pubInEX, pubLog, resultsLog);

            acknowledgeQOS2(input, output, pubLog, resultsLog, clearInputStream);
        }
        else
        {
            // clear the input stream to discard any more copies of the publish message
            if (clearInputStream)
            {
                input.skipBytes(input.available());
            }

            // The msg is Qos0 - therefore we just compare

            SCADAutils.compareAndLog(this, pubInEX, pubLog, resultsLog);

        }

    }

    // Works out the length of variable part of msg
    private int variableLength()
    {

        int result = 0;

        result += payload.length;
        result += topic.length;

        if (myQos == 1 || myQos == 2 || myQos == 3) // added QOS3 option for SDP_PROTOCOL_ERRORS test
        {
            // Need 2 bytes for msgID
            result += 2;
        }

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

        for (int i = 0; i < topic.length; i++)
        {
            construct.addElement(new Byte(topic[i]));
        }

        // If Qos not equal to 0 add the msgId

        if (myQos != 0)
        {
            for (int i = 0; i < msgId.length; i++)
            {
                construct.addElement(new Byte(msgId[i]));
            }
        }

        for (int i = 0; i < payload.length; i++)
        {
            construct.addElement(new Byte(payload[i]));
        }

        if (construct.size() != size)
        {
            ScadaException myE = new ScadaException("Exception: Size mismatch when constructing msg in function PUBLISHmsg buildMsg() - construct is " + construct.size() + "whilst message is " + size);
            throw (myE);
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
    @Override
    protected void setFields()
    {

        int offset = 1 + remainingLength.length;

        myQos = SCADAutils.getQos(fixedHeader);

        byte topicLenByte0 = completeMsg[offset];
        offset++;
        byte topicLenByte1 = completeMsg[offset];
        offset++;

        // Copying both len bytes has been carried over from the original test case and 
        // appears to be a crude way of reversing the bytes required for computing the length
        byte[] topicNameLength = new byte[2];
        topicNameLength[0] = topicLenByte0;
        topicNameLength[0] = topicLenByte1;
        
        int topicLength = SCADAutils.remainingLengthAsDecimal(topicNameLength);
        topicLength += 2;

        topic = new byte[topicLength];

        topic[0] = topicLenByte0;
        topic[1] = topicLenByte1;

        for (int i = 2; i < topic.length; i++)
        {
            topic[i] = completeMsg[offset];
            offset++;
        }

        if (myQos != 0)
        {
            // Then we have a msgid
            msgId = new byte[2];
            msgId[0] = completeMsg[offset];
            offset++;
            msgId[1] = completeMsg[offset];
            offset++;
            myId = new MsgIdentifier(msgId);

        }

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

    // Use this function to publish a message and have the
    // acknowledgements for Qos1 and 2 messages handled
    // automatically - if you want more control, use the
    // standard send function which just puts the message on
    // the output stream

    public void sendAndAcknowledge(DataInputStream input, DataOutputStream output, LogFile pubLog, LogFile resultsLog) throws IOException, ScadaException
    {

        if (myQos == 1)
        {

            send(output, pubLog);

            resultsLog.println("Message sent at Qos 1 - waiting for reply");

            PUBACKmsg pubAck = new PUBACKmsg(input);
            PUBACKmsg pubAckEX = new PUBACKmsg(myId);

            SCADAutils.compareAndLog(pubAck, pubAckEX, pubLog, resultsLog);
        }
        else if (myQos == 2)
        {

            send(output, pubLog);

            resultsLog.println("Message sent at Qos 2 - waiting for reply");

            PUBRECmsg pubRec = new PUBRECmsg(input);
            PUBRECmsg pubRecEX = new PUBRECmsg(myId);

            SCADAutils.compareAndLog(pubRec, pubRecEX, pubLog, resultsLog);

            // Send next msg in chain - PUBREL

            resultsLog.println("Creating PUBREL message - QoS 2");

            PUBRELmsg pubRel = new PUBRELmsg(myId);

            pubLog.printTitle("PUBREL msg sent to broker (Qos2):");
            pubRel.parseAndPrint(pubLog);
            resultsLog.println("PUBREL message created in " + pubLog.name);
            resultsLog.println("Sending PUBREL message to broker...");

            pubRel.send(output, pubLog);

            resultsLog.println("Message sent - waiting for reply");

            PUBCOMPmsg pubComp = new PUBCOMPmsg(input);
            PUBCOMPmsg pubCompEX = new PUBCOMPmsg(myId);

            SCADAutils.compareAndLog(pubComp, pubCompEX, pubLog, resultsLog);

        }
        else
        {

            // The msg is Qos0 - therefore we do a simple send();

            send(output, pubLog);
        }

    }

    // Sets the DUP flag of the message
    public void setDUP()
    {

        fixedHeader = SCADAutils.setDUP(fixedHeader);
        completeMsg[0] = fixedHeader;
    }

    // Sets the QoS of the message to the level specified
    public void setQos(byte qos) throws Exception
    {

        fixedHeader = SCADAutils.setQos(fixedHeader, qos);
        completeMsg[0] = fixedHeader;
    }

    // Sets the ID of the message using the MsgIdentifier supplied
    public void setId(MsgIdentifier id) throws ScadaException
    {

        msgId = id.msgId;
        myId = id;
        buildMsg();
    }

    // Sets the RETAIN flag of the message
    public void setRetain()
    {

        fixedHeader = SCADAutils.setRetain(fixedHeader);
        completeMsg[0] = fixedHeader;
    }

    // Run through and prints out message byte information
    @Override
    public void parseAndPrint(LogFile log) throws IOException
    {
        super.parseAndPrint(log);

        int offset = 1 + remainingLength.length;

        byte[] topicNameLength = new byte[2];

        // check to see if this is an incomplete message test from SDP_PROTOCOL_NETWORK
        if (offset >= completeMsg.length)
        {
            return;
        }
        
        SCADAutils.printByteNumeric(completeMsg[offset], log, "TOPIC NAME LENGTH(1)", offset);
        topicNameLength[0] = completeMsg[offset];
        offset++;
        SCADAutils.printByteNumeric(completeMsg[offset], log, "TOPIC NAME LENGTH(2)", offset);
        topicNameLength[0] = completeMsg[offset];
        offset++;

        int topicLength = SCADAutils.remainingLengthAsDecimal(topicNameLength);

        for (int i = 0; i < topicLength; i++)
        {
            SCADAutils.printByteChar(completeMsg[offset], log, "TOPIC NAME", offset);
            offset++;
        }

        if (myQos != 0)
        {
            // Then we have a msgid
            SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID", offset);
            offset++;
            SCADAutils.printByteNumeric(completeMsg[offset], log, "MSG ID", offset);
            offset++;

        }

        for (; offset < completeMsg.length; offset++)
        {
            SCADAutils.printByteChar(completeMsg[offset], log, "PAYLOAD", offset);
        }
    }
    
    public void acknowledgeQOS1(DataInputStream input, DataOutputStream output, LogFile pubLog, LogFile resultsLog, boolean clearInputStream) throws IOException
    {
        resultsLog.println("Message received is QoS1 - preparing required PUBACK msg");

        PUBACKmsg pubAck = new PUBACKmsg(myId);

        resultsLog.println("PUBACK message created in " + pubLog.name);
        resultsLog.println("PUBACK created - sending to broker");
        pubLog.printTitle("PUBACK sent to broker:");
        pubAck.parseAndPrint(pubLog);

        // clear the input stream to discard any more copies of the publish message
        if (clearInputStream)
        {
            input.skipBytes(input.available());
        }

        pubAck.send(output, pubLog);

        resultsLog.println("Message sent!");
    }
    
    public void acknowledgeQOS2(DataInputStream input, DataOutputStream output, LogFile pubLog, LogFile resultsLog, boolean clearInputStream) throws IOException, ScadaException
    {
        resultsLog.println("Message received is QoS2 - preparing required PUBREC msg");

        PUBRECmsg pubRec = new PUBRECmsg(myId);

        resultsLog.println("PUBREC created - sending to broker");
        pubLog.printTitle("PUBREC sent to broker:");
        pubRec.parseAndPrint(pubLog);

        // clear the input stream to discard any more copies of the publish message
        if (clearInputStream)
        {
            input.skipBytes(input.available());
        }

        pubRec.send(output, pubLog);

        resultsLog.println("Message sent! - waiting for PUBREL reply...");
        resultsLog.println("Message sent at Qos 2 - waiting for reply");

        PUBRELmsg pubRel = new PUBRELmsg(input);
        PUBRELmsg pubRelEX = new PUBRELmsg(myId);

        SCADAutils.compareAndLog(pubRel, pubRelEX, pubLog, resultsLog);

        // Send next msg in chain - PUBCOMP

        resultsLog.println("Creating next reply - PUBCOMP message");

        PUBCOMPmsg pubComp = new PUBCOMPmsg(myId);

        pubLog.printTitle("PUBCOMP msg sent to broker (Qos2):");
        pubComp.parseAndPrint(pubLog);
        resultsLog.println("PUBCOMP message created in " + pubLog.name);
        resultsLog.println("Sending PUBCOMP message to broker...");

        pubComp.send(output, pubLog);

        resultsLog.println("Message sent!");
    }
}
