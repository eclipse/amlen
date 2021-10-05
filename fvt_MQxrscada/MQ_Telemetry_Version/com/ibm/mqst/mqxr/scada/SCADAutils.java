package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

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
 * This class contains functions used in the creation and manipulation of SDP
 * messages, and also common routines used in test case classes
 */

public final class SCADAutils
{

    // Useful constants

    // Protocol name and length
    final static byte[] protocolName = { 'M', 'Q', 'I', 's', 'd', 'p' };

    // Version
    final static byte version = 0x03;

    // IO Timeout
    final static int timeout = 30000;

    // Connect msg fixed header - this never changes
    final static byte fixedHeaderCONNECT = 0x10;

    // Subscribe msg fixed header - the first subscribe sent will ALWAYS have
    // this header
    final static byte fixedHeaderSUBSCRIBE = (byte) 0x82;

    // Subroutines
    /** ***************************************************** */
    // Prints a byte in binary representation e.g. 11011011
    public final static void printBinary(byte b, LogFile log)
    {
        for (int i = 7; i >= 0; i--)
        {
            log.print((b >> i) & 1);
        }
    }

    /** ***************************************************** */
    // Prints a formatted binary representation e.g. [11011011]
    public final static String binaryString(byte b)
    {
        String result = "[";
        for (int i = 7; i >= 0; i--)
        {
            int tmp = (b >> i) & 1;
            result = result.concat(new Integer(tmp).toString());
        }

        result = result.concat("]");
        return result;
    }

    /** ***************************************************** */
    // Converts a string to UTF-8 format
    public final static byte[] StringToUTF(String Data)
    {
        if (Data == null)
            return (null);
        byte c[] = new byte[Data.length() + 2];

        c[0] = new Integer(Data.length() / 256).byteValue();
        c[1] = new Integer(Data.length() % 256).byteValue();

        System.arraycopy(Data.getBytes(), 0, c, 2, Data.length());

        return (c); /* return converted array */
    }

    /** ***************************************************** */
    // Sets bits in a byte individually
    public final static byte setBits(int a, int b, int c, int d, int e, int f, int g, int h)
    {
        byte result = 0;

        if (h == 1)
        {
            result |= 1;
        }
        if (g == 1)
        {
            result |= 2;
        }
        if (f == 1)
        {
            result |= 4;
        }
        if (e == 1)
        {
            result |= 8;
        }
        if (d == 1)
        {
            result |= 16;
        }
        if (c == 1)
        {
            result |= 32;
        }
        if (b == 1)
        {
            result |= 64;
        }
        if (a == 1)
        {
            result |= 128;
        }

        return result;
    }

    /** ***************************************************** */
    // Converts a 16bit short into 2 8bit bytes e.g. for Msg IDS
    public final static byte[] shortToBytes(short sht)
    {

        byte[] result = new byte[2];
        result[0] = (byte) (sht / 256);
        result[1] = (byte) (sht % 256);
        return result;

    }

    /** ***************************************************** */
    // Converts a 2 byte UTF-8 String length into a normal decimal value
    public final static short UTFLengthAsDecimal(byte[] bytes)
    {

        byte tmp1 = bytes[0];
        byte tmp2 = bytes[1];

        short result = 0;
        result += (tmp1 * 256) + tmp2;
        return result;

    }

    /** ***************************************************** */
    // Sets the remaining length field of SDP messages
    public final static byte[] setRemainingLength(int size)
    {

        byte[] maxBytes = new byte[4];
        int sizeTmp = size;
        int counter = 0;

        do
        {
            maxBytes[counter] = (byte) (sizeTmp % 128);
            sizeTmp = sizeTmp / 128;
            // If there are more bytes needed set the top bit of this one
            if (sizeTmp > 0)
            {
                maxBytes[counter] |= 0x80;
            }
            counter++;
        }
        while (sizeTmp > 0);

        byte[] result = new byte[counter];
        for (int i = 0; i < counter; i++)
        {
            result[i] = maxBytes[i];
        }
        return result;
    }

    // As above but reads dynamically from the input stream

    public final static byte[] setRemainingLength(DataInputStream in) throws IOException
    {

        byte[] maxBytes = new byte[4];
        int counter = 0;

        maxBytes[counter] = in.readByte();

        while ((maxBytes[counter] & 0x80) > 0)
        {
            counter++;
            maxBytes[counter] = in.readByte();

        }

        byte[] result = new byte[counter + 1];
        for (int i = 0; i < counter + 1; i++)
        {
            result[i] = maxBytes[i];
        }
        return result;
    }

    /** ***************************************************** */
    // Converts the remaining length byte array to a normal decimal value
    public final static int remainingLengthAsDecimal(byte[] vLength)
    {

        int multiplier = 1;
        int value = 0;
        for (int i = 0; i < vLength.length; i++)
        {
            value += (vLength[i] & 0x7F) * multiplier;
            multiplier *= 128;
        }

        return value;
    }

    /** ***************************************************** */
    // Return the string representation of a SDP Connect return code
    public static String connectReply(byte returnCode)
    {

        switch (returnCode)
        {
        case 0:
            return "Connection accepted";
        case 1:
            return "Connection refused: unacceptable protocol version";
        case 2:
            return "Connection refused: identifier rejected";
        case 3:
            return "Connection refused: broker unavailable";
        default:
            return "Connection refused: unknown return code";

        }
    }

    /** ***************************************************** */
    // Returns the string representation of the SDP message types
    public static String getMsgType(byte header)
    {

        byte realValue = header;
        // realValue &= header;
        realValue = (byte) (realValue >>> 4);
        realValue &= 0x0f;
        switch (realValue)
        {
        case Headers.RESERVED:
            return "Reserved";
        case Headers.CONNECT:
            return "CONNECT";
        case Headers.CONNACK:
            return "CONNACK";
        case Headers.PUBLISH:
            return "PUBLISH";
        case Headers.PUBACK:
            return "PUBACK";
        case Headers.PUBREC:
            return "PUBREC";
        case Headers.PUBREL:
            return "PUBREL";
        case Headers.PUBCOMP:
            return "PUBCOMP";
        case Headers.SUBSCRIBE:
            return "SUBSCRIBE";
        case Headers.SUBACK:
            return "SUBACK";
        case Headers.UNSUBSCRIBE:
            return "UNSUBSCRIBE";
        case Headers.UNSUBACK:
            return "UNSUBACK";
        case Headers.PINGREQ:
            return "PINGREQ";
        case Headers.PINGRESP:
            return "PINGRESP";
        case Headers.DISCONNECT:
            return "DISCONNECT";
        case Headers.RESERVED2:
            return "Reserved";
        default:
            return "Unknown msg type";
        }
    }

    /** ***************************************************** */
    // Checks the header byte to see if the DUP flag is set
    public static String isDupSet(byte header)
    {

        byte realValue = (byte) 0x08;
        realValue &= header;
        realValue = (byte) (realValue >> 3);
        if (realValue == 1)
        {
            return "YES";
        }
        else if (realValue == 0)
        {
            return "NO";
        }
        else
        {
            return "UNKNOWN";
        }
    }

    /** ***************************************************** */
    // Sets the DUP flag in a header and returns the modified byte
    public static byte setDUP(byte header)
    {

        byte result = (byte) 0x08;
        result |= header;
        return result;
    }

    /** ***************************************************** */
    // Returns the QoS from the message header as a String representation for
    // use in printing
    public static String getQosAsString(byte header)
    {

        byte realValue = (byte) 0x06;
        realValue &= header;
        realValue = (byte) (realValue >> 1);
        switch (realValue)
        {
        case 0:
            return "0";
        case 1:
            return "1";
        case 2:
            return "2";
        case 3:
            return "3 (Reserved)";
        default:
            return "INVALID QOS";
        }
    }

    /** ***************************************************** */
    // Returns the byte representation of the message QoS
    public static byte getQos(byte header)
    {

        byte realValue = (byte) 0x06;
        realValue &= header;
        realValue = (byte) (realValue >> 1);
        return realValue;
    }

    /** ***************************************************** */
    // Modifies the header to set the supplied QoS level
    public static byte setQos(byte header, int qos) throws ScadaException
    {
        byte result;

        if (qos == 0)
        {
            result = setBits(1, 1, 1, 1, 1, 0, 0, 1);
            result &= header;
        }
        else if (qos == 1)
        {
            result = setBits(0, 0, 0, 0, 0, 0, 1, 0);
            result |= header;
        }
        else if (qos == 2)
        {
            result = setBits(0, 0, 0, 0, 0, 1, 0, 0);
            result |= header;
        }
        else if (qos == 3) // this is an invalid value but we want to test it is handled correctly
        {
            result = setBits(0, 0, 0, 0, 0, 1, 1, 0);
            result |= header;
        }
        else
        {
            ScadaException myE = new ScadaException("Attempt to set Qos out of bounds - " + qos + " used");
            throw myE;
        }

        return result;
    }

    /** ***************************************************** */
    // Checks if the RETAIN flag in a message header is set
    public static String isRetainSet(byte header)
    {

        byte realValue = (byte) 0x01;
        realValue &= header;
        if (realValue == 1)
        {
            return "YES";
        }
        else if (realValue == 0)
        {
            return "NO";
        }
        else
        {
            return "UNKNOWN";
        }
    }

    /** ***************************************************** */
    // Returns a numeric value for the WILL flag in the connect
    // flags byte of a CONNECT msg
    public static byte getWillFlag(byte flags)
    {

        byte realValue = (byte) 0x04;
        realValue &= flags;
        realValue = (byte) (realValue >> 2);
        return realValue;
    }

    /** ***************************************************** */
    // Returns a numeric value for the WILL RETAIN flag in the connect
    // flags byte of a CONNECT msg
    public static byte getWillRetain(byte flags)
    {

        byte realValue = (byte) 0x20;
        realValue &= flags;
        realValue = (byte) (realValue >> 5);
        return realValue;

    }

    /** ***************************************************** */
    // Returns a numeric value for the WILL QoS in the connect
    // flags byte of a CONNECT msg
    public static byte getWillQos(byte flags)
    {

        byte realValue = (byte) 0x18;
        realValue &= flags;
        realValue = (byte) (realValue >> 3);
        return realValue;
    }

    /** ***************************************************** */
    // Compares two messages using the byte array of each message
    // and logs the results in the supplied log file
    public static boolean isEqual(byte[] msg1, byte[] msg2, LogFile log)
    {

        boolean result = true;

        if (msg1.length != msg2.length)
        {
            result = false;
            log.println("Messages are different lengths! Got " + msg1.length + ", expected " + msg2.length);
        }
        else
        {
            for (int i = 0; i < msg1.length; i++)
            {
                if (msg1[i] != msg2[i])
                {
                    result = false;
                    log.println("Messages differ at byte " + i + "! Got " + binaryString(msg1[i]) + " expected " + binaryString(msg2[i]));
                    break;
                }

            }
        }

        return result;

    }

    /** ***************************************************** */
    // Compares a message read from the data stream to the supplied
    // message (including the header QoS value) and logs the results 
    public static synchronized void compareAndLog(MsgTemplate msgIn, 
                                                  MsgTemplate msgEx, 
                                                  LogFile local, 
                                                  LogFile resultsLog) 
                                    throws IOException, ScadaException
    {
        compareAndLog(msgIn, msgEx, local, resultsLog, false);
    }

    /** ***************************************************** */
    // Compares a message read from the data stream to the supplied
    // message and logs the results. Optionally, ignore any differences
    // in the header QoS bytes, which are unused in some message 
    public static synchronized void compareAndLog(MsgTemplate msgIn, 
                                                  MsgTemplate msgEx, 
                                                  LogFile local, 
                                                  LogFile resultsLog, 
                                                  boolean ignoreQoS )
                                    throws IOException, ScadaException
    {

        local.printTitle(getMsgType(msgIn.fixedHeader) + " from broker:");
        msgIn.parseAndPrint(local);
        local.printTitle("Expected " + getMsgType(msgEx.fixedHeader) + 
                         " msg from broker:");
        msgEx.parseAndPrint(local);

        resultsLog.println(getMsgType(msgIn.fixedHeader) + 
                           " msg recieved - logged in " + local.name);
        resultsLog.println("Checking reply matches expected " + 
                           getMsgType(msgEx.fixedHeader) + " reply");

        // if we don't care about the QoS bits in the headers (e.g. when 
        // comparing SUBACKs), set the expected QoS to the same as the 
        // received QoS
        // 
        if (ignoreQoS)
        {
            resultsLog.println("Ignoring QoS bits in header.");
            msgEx.fixedHeader = SCADAutils.setQos(msgEx.fixedHeader, 
                                  SCADAutils.getQos(msgIn.fixedHeader));
            // replace the header in the complete message byte array
            msgEx.completeMsg[0] = msgEx.fixedHeader;
        }

        // Check response
        if (!isEqual(msgIn.completeMsg, msgEx.completeMsg, local))
        {
            resultsLog.println("Response did not match expected response");
            throw new ScadaException(getMsgType(msgEx.fixedHeader) + 
                                     " message invalid - see " + 
                                     local.name + " for details");
        }
        else
        {
            resultsLog.println("Messages match!");
        }
    }

    /** ***************************************************** */
    // Compares a message read from the data stream to the supplied
    // message and logs the results
    public static synchronized void compareAndLog(ArrayList msgInList, ArrayList msgExList, LogFile local, LogFile resultsLog, DataInputStream input, DataOutputStream output, boolean acknowledge) throws IOException, ScadaException
    {
        StringBuffer exceptionText = new StringBuffer();
        ArrayList unmatchedReceivedMsgs = (ArrayList) msgInList.clone();
        ArrayList unmatchedExpectedMsgs = (ArrayList) msgExList.clone();
        for (Iterator expectedIterator = msgExList.iterator(); expectedIterator.hasNext();)
        {
            MsgTemplate msgEx = (MsgTemplate) expectedIterator.next();
            local.printTitle("Expected " + getMsgType(msgEx.fixedHeader) + " msg from broker:");
            
            MsgTemplate msgIn = null;
            for (Iterator receivedIterator = msgExList.iterator(); receivedIterator.hasNext();)
            {
                MsgTemplate tempMsg = (MsgTemplate) receivedIterator.next();
                if ((tempMsg.fixedHeader == msgEx.fixedHeader) && tempMsg.completeMsg.equals(msgEx.completeMsg))
                {
                    msgIn = tempMsg;
                    break;
                }
            }
            
            if (msgIn == null)
            {
                exceptionText.append(getMsgType(msgEx.fixedHeader) + " not found in list of messages- see " + local.name + " for details\n");
                continue;
            }

            local.printTitle(getMsgType(msgIn.fixedHeader) + " from broker:");
            msgIn.parseAndPrint(local);
            local.printTitle("Expected " + getMsgType(msgEx.fixedHeader) + " msg from broker:");
            msgEx.parseAndPrint(local);
    
            resultsLog.println(getMsgType(msgIn.fixedHeader) + " msg recieved - logged in " + local.name);
            resultsLog.println("Checking reply matches expected " + getMsgType(msgEx.fixedHeader) + " reply");
    
            // Check response
            if (!isEqual(msgIn.completeMsg, msgEx.completeMsg, local))
            {
                resultsLog.println("Response did not match expected response");
                exceptionText.append(getMsgType(msgEx.fixedHeader) + " message invalid - see " + local.name + " for details\n");
            }
            else
            {
                resultsLog.println("Messages match!");
            }

            unmatchedExpectedMsgs.remove(msgEx);
            unmatchedReceivedMsgs.remove(msgIn);
            
            if (acknowledge && msgIn instanceof PUBLISHmsg)
            {
                PUBLISHmsg publishMsg = (PUBLISHmsg) msgIn;
                if (publishMsg.myQos == 1)
                {
                    publishMsg.acknowledgeQOS1(input, output, local, resultsLog, false);
                }
                else if (publishMsg.myQos == 2)
                {
                    publishMsg.acknowledgeQOS2(input, output, local, resultsLog, false);
                }
            }
        }
        
        if (unmatchedExpectedMsgs.size() > 0)
        {
            local.printTitle("The following messages did not match:");
            for (Iterator expectedIterator = unmatchedExpectedMsgs.iterator(); expectedIterator.hasNext();)
            {
                MsgTemplate msgEx = (MsgTemplate) expectedIterator.next();
                local.printTitle("Expected " + getMsgType(msgEx.fixedHeader) + " msg from broker:");
                msgEx.parseAndPrint(local);
            }
            for (Iterator receivedIterator = unmatchedReceivedMsgs.iterator(); receivedIterator.hasNext();)
            {
                MsgTemplate msgIn = (MsgTemplate) receivedIterator.next();
                local.printTitle("Received " + getMsgType(msgIn.fixedHeader) + " msg from broker:");
                msgIn.parseAndPrint(local);
            }
        }
        
        if (exceptionText.length() > 0)
        {
            throw new ScadaException(exceptionText.toString());
        }
    }

    /** ***************************************************** */
    // Sets the RETAIN flag of a msg header
    public static byte setRetain(byte header)
    {

        byte result = setBits(0, 0, 0, 0, 0, 0, 0, 1);
        result |= header;
        return result;

    }

    /** ***************************************************** */
    // Sets the WILL QoS level in the connect flags byte of
    // a CONNECT msg to the specified value
    public static byte setWillQos(byte flags, int qos) throws ScadaException
    {
        byte result;

        if (qos == 0)
        {
            result = setBits(1, 1, 1, 0, 0, 1, 1, 1);
            result &= flags;
        }
        else if (qos == 1)
        {
            result = setBits(0, 0, 0, 0, 1, 0, 0, 0);
            result |= flags;
        }
        else if (qos == 2)
        {
            result = setBits(0, 0, 0, 1, 1, 0, 0, 0);
            result |= flags;
        }
        else if (qos == 3)
        {
            ScadaException myE = new ScadaException("Attempt to set Will Qos level to 3 - reserved value");
            throw myE;
        }
        else
        {
            ScadaException myE = new ScadaException("Attempt to set Will Qos out of bounds - " + qos + " used");
            throw myE;
        }

        return result;
    }

    /** ***************************************************** */
    // Sets the WILL RETAIN flag of the conect flags byte in
    // a CONNECT msg to ON
    public static byte setWillRetain(byte flags)
    {

        byte result = setBits(0, 0, 1, 0, 0, 0, 0, 0);
        result |= flags;
        return result;

    }

    /** ***************************************************** */
    // Queries if the CLEAN START flag is set in the supplied byte
    public static String isCleanStartSet(byte flags)
    {
        byte tmp = SCADAutils.setBits(0, 0, 0, 0, 0, 0, 1, 0);
        tmp &= flags;
        if (tmp == 2)
        {
            return "YES";
        }
        else
        {
            return "NO";
        }
    }

    /** ***************************************************** */
    // Queries if the WILL flag is set in the supplied byte
    public static String isWillFlagSet(byte flags)
    {
        byte tmp = 0x04;
        tmp &= flags;
        if (tmp == 4)
        {
            return "YES";
        }
        else
        {
            return "NO";
        }
    }

    /** ***************************************************** */
    // Queries if the WILL RETAIN flag is set in the supplied byte
    public static String isWillRetainFlagSet(byte flags)
    {
        byte tmp = 0x20;
        tmp &= flags;
        if (tmp == 32)
        {
            return "YES";
        }
        else
        {
            return "NO";
        }
    }

    /** ***************************************************** */
    // Converts a UTF string back to a normal Java string object
    public static String UTFtoString(byte[] utf)
    {
        int size = utf.length;
        size -= 2;
        byte[] tmp = new byte[size];

        for (int i = 2; i < utf.length; i++)
        {
            tmp[i - 2] = utf[i];
        }
        return new String(tmp);
    }

    /** ***************************************************** */
    // Used in parseAndPrint functions:
    // Displays a formatted line containing a byte in binary
    // representation, and the character spcified by that byte
    public static void printByteChar(byte data, LogFile log, String byteName, int byteNum)
    {
        log.print("BYTE[" + byteNum + "] =\t");
        SCADAutils.printBinary(data, log);
        log.print("\t\t" + byteName + "(" + (char) data + ")");
        log.newLine();
    }

    /** ***************************************************** */
    // Used in parseAndPrint functions:
    // Displays a formatted line containing a byte in binary
    // representation, and the numeric value of that byte
    public static void printByteNumeric(byte data, LogFile log, String byteName, int byteNum) 
    {
        log.print("BYTE[" + byteNum + "] =\t");
        SCADAutils.printBinary(data, log);
        log.print("\t\t" + byteName + "(" + data + ")");
        log.newLine();
    }
    /** ***************************************************** */
    

    public static void sleep(long sleepTime)
    {
        Date endTime = new Date(new Date().getTime() + sleepTime);
        boolean interrupted = true;
        while (interrupted)
        {
            try
            {
                Thread.sleep(sleepTime);
                interrupted = false;
            }
            catch (InterruptedException e)
            {
                e.printStackTrace();
                Date currentTime = new Date();
                if (currentTime.before(endTime))
                {
                    sleepTime = endTime.getTime() - currentTime.getTime();
                }
                else
                {
                    interrupted = false;
                }
            }
        }
    }
    
    public static MsgTemplate getGenericMsg(DataInputStream in) throws IOException, ScadaException
    {
        byte header;
        try
        {
            header = in.readByte();
        }
        catch (EOFException e)
        {
            // ignore first EOFException, sleep for 10 seconds and try again- message may not have arrived yet
            SCADAutils.sleep(10000);
            header = in.readByte();
        }
        
        byte realValue = header;
        realValue = (byte) (realValue >>> 4);
        realValue &= 0x0f;
        switch (realValue)
        {
        case Headers.CONNECT:
            return new CONNECTmsg(in, false);
        case Headers.CONNACK:
            return new CONNACKmsg(in, false);
        case Headers.PUBLISH:
            return new PUBLISHmsg(in, false);
        case Headers.PUBACK:
            return new PUBACKmsg(in, false);
        case Headers.PUBREC:
            return new PUBRECmsg(in, false);
        case Headers.PUBREL:
            return new PUBRELmsg(in, false);
        case Headers.PUBCOMP:
            return new PUBCOMPmsg(in, false);
        case Headers.SUBSCRIBE:
            return new SUBSCRIBEmsg(in, false);
        case Headers.SUBACK:
            return new SUBACKmsg(in, false);
        case Headers.UNSUBSCRIBE:
            return new UNSUBSCRIBEmsg(in, false);
        case Headers.UNSUBACK:
            return new UNSUBACKmsg(in, false);
        case Headers.PINGREQ:
            return new PINGREQmsg(in, false);
        case Headers.PINGRESP:
            return new PINGRESPmsg(in, false);
        case Headers.DISCONNECT:
            return new DISCONNECTmsg(in, false);
        default:
            throw new ScadaException("Unknown msg type");
        }
    }
    
    public static void enableListenerViaDifferentFlow(int portToEnable, int flowPort, LogFile resultsLog, LogFile connectLog, LogFile pubLog ) throws IOException, ScadaException
    {
        Client client = new Client("localhost", flowPort);
        String topic = "$SYS/SCADA/MQIsdpListener/" + portToEnable;
        byte[] payload = "ON".getBytes();
        
        CONNECTmsg connectMessage = new CONNECTmsg(client.getClientId());
        resultsLog.println("Message created in " + connectLog.name);
        resultsLog.println("Sending CONNECT mesasge to broker for subscribe");
        connectMessage.sendAndAcknowledge(client.getInputStream(), client.getOutputStream(), connectLog, resultsLog);

        PUBLISHmsg publish = new PUBLISHmsg(topic, payload);
        resultsLog.println("Enable message published for port " + portToEnable);
        publish.send(client.getOutputStream(), pubLog);

    }
}
