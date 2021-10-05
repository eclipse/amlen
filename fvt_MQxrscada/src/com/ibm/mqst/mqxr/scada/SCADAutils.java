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
package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;

import org.apache.commons.lang3.RandomStringUtils;

/*
 * This class contains functions used in the creation and manipulation of SDP
 * messages, and also common routines used in test case classes
 */

public final class SCADAutils
{

    // Useful constants

    // Protocol name and length
    static byte[] protocolName31 = { 'M', 'Q', 'I', 's', 'd', 'p' };
    static byte[] protocolName311 = { 'M', 'Q', 'T', 'T' };
    
    static byte[] protocolName = protocolName31;

	// Version
    static byte version31 = 0x03;
    static byte version311 = 0x04;
    static byte version5 = 0x05;
    static byte version = version31;

	// IO Timeout
    final static int timeout = 1000;

    // Connect msg fixed header - this never changes
    final static byte fixedHeaderCONNECT = 0x10;

    // Subscribe msg fixed header - the first subscribe sent will ALWAYS have
    // this header
    final static byte fixedHeaderSUBSCRIBE = (byte) 0x82;

    // Subroutines
    /** ***************************************************** */
    // Set the default protocol name
    public static void setProtocolName(String protocolName) {
		SCADAutils.protocolName = protocolName.getBytes();
	}

    /** ***************************************************** */
    // Set the default protocol version number
    public static void setVersion(byte version) {
		SCADAutils.version = version;
		if (3 == version) {
			protocolName = protocolName31;
			System.out.println("Setting MQTT version for test to 3.1");
		} else if (4 == version) {
			protocolName = protocolName311;
			System.out.println("Setting MQTT version for test to 3.1.1");
		} else if (5 == version) {
            protocolName = protocolName311;
            System.out.println("Setting MQTT version for test to 5");
		}    
	}

	public static byte getVersion() {
		return version;
	}

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
    // Converts a string to UTF-8 format\
    // Used to assume everything was ASCII7. Modify to accept any valid Unicode
    public final static byte[] StringToUTF(String Data)
    {
        if (Data == null)
            return (null);
    	byte data[] = null;
    	try {
    		data = Data.getBytes("UTF-8");
    	} catch (UnsupportedEncodingException e) {
    		// Should never get here
    	}
        byte c[] = new byte[data.length + 2];

        c[0] = new Integer(data.length / 256).byteValue();
        c[1] = new Integer(data.length % 256).byteValue();

        System.arraycopy(data, 0, c, 2, data.length);

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
    // Converts the remaining length byte array to a normal decimal value
    public final static int remainingLength(byte[] vLength, int offset)
    {

        int multiplier = 1;
        int value = 0;
        for (int i = offset; i < vLength.length; i++)
        {
            value += (vLength[i] & 0x7F) * multiplier;
            multiplier *= 128;
            if ((vLength[i] & 0x80) == 0)
                break;
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
            return "Connection refused: unknown return code "+returnCode;

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
        case Headers.AUTH:
            return "AUTH";
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
    // Returns a numeric value for the User flag in the connect
    // flags byte of a CONNECT msg
    public static byte getUserFlag(byte flags)
    {

        byte realValue = (byte) 0x80;
        realValue &= flags;
        realValue = (byte) (1 & (realValue >> 7));
        return realValue;
    }

    /** ***************************************************** */
    // Returns a numeric value for the Password flag in the connect
    // flags byte of a CONNECT msg
    public static byte getPasswordFlag(byte flags)
    {

        byte realValue = (byte) 0x40;
        realValue &= flags;
        realValue = (byte) (realValue >> 6);
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
        if (ignoreQoS && SCADAutils.version < 4)
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
            boolean matched = false;            
            if (SCADAutils.version >= 5) {
                if (msgIn.fixedHeader == msgEx.fixedHeader) {
                    int irlen = remainingLength(msgIn.completeMsg, 1);
                    int xrlen = remainingLength(msgEx.completeMsg, 1);
                    int ipos = 2;
                    int xpos = 2;
                    if (irlen > 127) 
                        ipos++;
                    if (xrlen > 127)
                        xpos++;
                    switch (msgIn.fixedHeader) {
                    case (byte)0x20:  /* CONNACK */
                        if (msgIn.completeMsg[ipos] == msgEx.completeMsg[xpos] && /* present */
                            msgIn.completeMsg[ipos+1] == msgEx.completeMsg[xpos+1]) /* rc */
                        matched = true;    
                        break;
                    case (byte)0x40:  /* PUBACK */ 
                    case (byte)0x50:  /* PUBREC */
                    case (byte)0x62:  /* PUBREL */
                    case (byte)0x70:  /* PUBCOMP */
                        if (msgIn.completeMsg[ipos] == msgEx.completeMsg[xpos] && /* packetID 0 */
                            msgIn.completeMsg[ipos+1] == msgEx.completeMsg[xpos+1] ) { /* packetID 1 */
                            int xrc = xrlen > 2 ? msgEx.completeMsg[xpos+2] : 0;
                            int irc = irlen > 2 ? msgIn.completeMsg[ipos+2] : 0;
                            if (irc == xrc)
                                matched = true;
                        }
                        break;
                    case (byte)0x90:  /* SUBACK */
                    case (byte)0xB0:  /* UNSUBACK */
                        if (msgIn.completeMsg[ipos] == msgEx.completeMsg[xpos] && /* packetID 0 */
                            msgIn.completeMsg[ipos+1] == msgEx.completeMsg[xpos+1] ) { /* packetID 1 */
                            
                            int xrc = msgEx.completeMsg[xpos+3];   /* TODO: Just look at one for now */
                            int irc = msgIn.completeMsg[ipos+3];
                            if (irc == xrc)
                                matched = true;
                        }    
                        break;
                    }
                }
            }
            if (!matched) {
                resultsLog.println("Response did not match expected response");
                throw new ScadaException(getMsgType(msgEx.fixedHeader) + 
                                     " message invalid - see " + 
                                     local.name + " for details");
            } else {
                resultsLog.println("Messages match well enough");
            }
        }
        else
        {
            resultsLog.println("Messages match!");
        }
    }

    private static boolean matchCompleteMsg2(MsgTemplate msg1, MsgTemplate msg2) {
    	if (msg1.completeMsg.length != msg2.completeMsg.length) return false;
    	for (int i=1; i<msg1.completeMsg.length; i++) {
    		if (msg1.completeMsg[i] != msg2.completeMsg[i]) return false;
    	}
    	return true;
    }
    
    
    /** ***************************************************** */
    // Compares an array of messages read from the data stream to the supplied
    // messages and logs the results
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
            byte mask = (byte)0xff;
            if (msgEx instanceof SUBACKmsg) {
            	mask = (byte)0xf9;
            }
            for (Iterator receivedIterator = unmatchedReceivedMsgs.iterator(); receivedIterator.hasNext();)
            {
                MsgTemplate tempMsg = (MsgTemplate) receivedIterator.next();
                if (((tempMsg.fixedHeader & mask) == (msgEx.fixedHeader & mask)) && matchCompleteMsg2(tempMsg, msgEx))
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
/*            if (!isEqual(msgIn.completeMsg, msgEx.completeMsg, local))
            {
                resultsLog.println("Response did not match expected response");
                exceptionText.append(getMsgType(msgEx.fixedHeader) + " message invalid - see " + local.name + " for details\n");
            }
            else
*/            {
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
            result = setBits(0, 0, 0, 1, 0, 0, 0, 0);
            result |= flags;
        }
        else if (qos == 3)
        {
            result = setBits(0, 0, 0, 1, 1, 0, 0, 0);
            result |= flags;
        }
        else
        {
            ScadaException myE = new ScadaException("Attempt to set Will Qos out of bounds - " + qos + " used");
            throw myE;
        }

        return result;
    }

    /** ***************************************************** */
    // Sets the WILL RETAIN flag of the connect flags byte in
    // a CONNECT msg to ON
    public static byte setWillRetain(byte flags)
    {

        byte result = setBits(0, 0, 1, 0, 0, 0, 0, 0);
        result |= flags;
        return result;

    }

    /** ***************************************************** */
    // Sets the User flag of the connect flags byte in
    // a CONNECT msg to ON
    public static byte setUser(byte flags)
    {

        byte result = (byte) 0x80;
        result |= flags;
        return result;
    }

    /** ***************************************************** */
    // Sets the Password flag of the connect flags byte in
    // a CONNECT msg to ON
    public static byte setPassword(byte flags)
    {

        byte result = (byte) 0x40;
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
    // Queries if the User flag is set in the supplied byte
    public static String isUserFlagSet(byte flags)
    {
        byte tmp = (byte)0x80;
        tmp &= flags;
        if (tmp != 0)
        {
            return "YES";
        }
        else
        {
            return "NO";
        }
    }

    /** ***************************************************** */
    // Queries if the Password flag is set in the supplied byte
    public static String isPasswordFlagSet(byte flags)
    {
        byte tmp = (byte)0x40;
        tmp &= flags;
        if (tmp != 0)
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
        log.print("\t\t" + byteName + "(" + (data&0xff));
        if (data > 0x20 && data < 0x7f)
            log.print(" " + (char)data + " )");
        else
            log.print(")");
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
    
    public static String randomString(int length, int maxSlash) {
    	String result = RandomStringUtils.random(length);
    	if (maxSlash < length) {
    		int position = 0;
    		int count = 0;
    		StringBuilder worker = new StringBuilder(result);
    		position = worker.indexOf("/", position+1);
    		while (position >= 0) {
    			count++;
    			if (count > maxSlash) {
    				worker.setCharAt(position, '.');
    			}
        		position = worker.indexOf("/", position+1);
    		}
    	}
    	return result;
    }
    
    public static String randomString(int length) {
    	return randomString(length, length);
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
        return getGenericMsg(in, header);
    }
    public static MsgTemplate getGenericMsg(DataInputStream in, byte header) throws IOException, ScadaException
    {     
        byte realValue = header;
        realValue = (byte) (realValue >>> 4);
        realValue &= 0x0f;
        switch (realValue)
        {
        case Headers.CONNECT:
            return new CONNECTmsg(in, header);
        case Headers.CONNACK:
            return new CONNACKmsg(in, header);
        case Headers.PUBLISH:
            return new PUBLISHmsg(in, header);
        case Headers.PUBACK:
            return new PUBACKmsg(in, header);
        case Headers.PUBREC:
            return new PUBRECmsg(in, header);
        case Headers.PUBREL:
            return new PUBRELmsg(in, header);
        case Headers.PUBCOMP:
            return new PUBCOMPmsg(in, header);
        case Headers.SUBSCRIBE:
            return new SUBSCRIBEmsg(in, header);
        case Headers.SUBACK:
            return new SUBACKmsg(in, header);
        case Headers.UNSUBSCRIBE:
            return new UNSUBSCRIBEmsg(in, header);
        case Headers.UNSUBACK:
            return new UNSUBACKmsg(in, header);
        case Headers.PINGREQ:
            return new PINGREQmsg(in, header);
        case Headers.PINGRESP:
            return new PINGRESPmsg(in, header);
        case Headers.DISCONNECT:
            return new DISCONNECTmsg(in, header);
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
        resultsLog.println("Sending CONNECT message to broker for subscribe");
        connectMessage.sendAndAcknowledge(client.getInputStream(), client.getOutputStream(), connectLog, resultsLog);

        PUBLISHmsg publish = new PUBLISHmsg(topic, payload);
        resultsLog.println("Enable message published for port " + portToEnable);
        publish.send(client.getOutputStream(), pubLog);

    }
}
