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

/* This class represents SDP message type CONNECT */

public class CONNECTmsg extends MsgTemplate
{
	public static final int MYTYPE = 1;

    byte[] protocolNameLength;

    byte[] protocolName;

    byte version;

    byte connectFlags;

    byte[] keepAlive;

    byte[] willTopic;

    byte[] willMessage;

    byte[] clientID;
    
    byte[] user;
    byte[] password;
    
    int extrabytes = 0;
    
    int expire = 0;

    // Parse constructor from superclass (see MsgTemplate.java)
    public CONNECTmsg(DataInputStream in) throws IOException
    {
        super(MYTYPE, in);
    }

    public CONNECTmsg(DataInputStream in, byte header) throws IOException
    {
        super(MYTYPE, in, header);
    }

    // Basic constructor - just need Client ID - clean start set to 1
    public CONNECTmsg(String clId) throws IOException, ScadaException
    {
        this(clId, true);
    }

    // Basic constructor - just need Client ID and cleanstart
    public CONNECTmsg(String clId, boolean cleanstart) throws IOException, ScadaException
    {
        this(clId, cleanstart, SCADAutils.protocolName, SCADAutils.version);
    }

    // Constructor to allow specifying of protocol name and version number
    public CONNECTmsg(String clId, boolean cleanstart, byte[] protocolName, byte version) throws IOException, ScadaException
    {
    	super(MYTYPE);
        fixedHeader = SCADAutils.fixedHeaderCONNECT;
        protocolNameLength = SCADAutils.shortToBytes((short) protocolName.length);
        this.protocolName = protocolName;
        this.version = version;
        connectFlags = SCADAutils.setBits(0, 0, 0, 0, 0, 0, cleanstart ? 1 : 0, 0);
        expire = cleanstart ? 0 : 3600;
        keepAlive = SCADAutils.shortToBytes((short) 0);
        clientID = SCADAutils.StringToUTF(clId);

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Advanced constructor - sets WILL information, cleanstart set to 1, add extra bytes
    public CONNECTmsg(String clId, String willTop, String willMes, int willQos, int extra) throws IOException, ScadaException
    {
        this(clId, willTop, willMes, willQos);
        extrabytes = extra;
        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Advanced constructor - sets WILL information, cleanstart set to 1
    public CONNECTmsg(String clId, String willTop, String willMes, int willQos) throws IOException, ScadaException
    {
        this(clId, willTop, willMes, willQos, true);
    }
    
    // Advanced constructor - sets WILL information and cleanstart
    public CONNECTmsg(String clId, String willTop, String willMes, int willQos, boolean cleanstart) throws IOException, ScadaException
    {
    	super(MYTYPE);
        fixedHeader = SCADAutils.fixedHeaderCONNECT;
        protocolNameLength = SCADAutils.shortToBytes((short) SCADAutils.protocolName.length);
        protocolName = SCADAutils.protocolName;
        version = SCADAutils.version;
        connectFlags = SCADAutils.setBits(0, 0, 0, 0, 0, 1, cleanstart ? 1 : 0, 0);
        keepAlive = SCADAutils.shortToBytes((short) 0);
        clientID = SCADAutils.StringToUTF(clId);
        willTopic = SCADAutils.StringToUTF(willTop);
        willMessage = SCADAutils.StringToUTF(willMes);
        connectFlags = SCADAutils.setWillQos(connectFlags, willQos);

        // Set remaining length
        remainingLength = SCADAutils.setRemainingLength(variableLength());

        // Build complete byte array
        buildMsg();
    }

    // Works out the length of variable part of msg
    private int variableLength()
    {

        int result = 0;

        result += protocolNameLength.length;
        result += protocolName.length;
        result += 1; // For version
        result += 1; // For connect flags
        result += keepAlive.length;
        result += clientID.length;
        
        if (SCADAutils.version >= 5) {
            if (expire == 999999999)
                result += 1;    /* zero length properties */
            else
                result += 6;    /* expire in properties */
        }

        if (SCADAutils.getWillFlag(connectFlags) == 1)
        {
            // Add the will information lengths
            if (SCADAutils.version >= 5) 
                result++;   /* No props */
            result += willTopic.length;
            result += willMessage.length;
        }

        if (SCADAutils.getUserFlag(connectFlags) == 1)
        {
        	result += user.length;
        }

        if (SCADAutils.getPasswordFlag(connectFlags) == 1)
        {
        	result += password.length;
        }
        result += extrabytes;
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

        for (int i = 0; i < protocolNameLength.length; i++)
        {
            construct.addElement(new Byte(protocolNameLength[i]));
        }

        for (int i = 0; i < protocolName.length; i++)
        {
            construct.addElement(new Byte(protocolName[i]));
        }

        construct.addElement(new Byte(version));
        construct.addElement(new Byte(connectFlags));

        for (int i = 0; i < keepAlive.length; i++)
        {
            construct.addElement(new Byte(keepAlive[i]));
        }
        
        if (SCADAutils.version >= 5) {
            if (expire == 999999999) {
                construct.addElement(new Byte((byte)0));
            } else {
                construct.addElement(new Byte((byte)5));    /* Proplen */
                construct.addElement(new Byte((byte)0x11)); /* Session Expiry */  
                construct.addElement(new Byte((byte)(expire>>24)));
                construct.addElement(new Byte((byte)(expire>>16)));
                construct.addElement(new Byte((byte)(expire>>8)));
                construct.addElement(new Byte((byte)(expire)));
            }
        }

        for (int i = 0; i < clientID.length; i++)
        {
            construct.addElement(new Byte(clientID[i]));
        }

        // Add WILL information if required
        if (SCADAutils.getWillFlag(connectFlags) == 1)
        {
            if (SCADAutils.version >= 5) 
                construct.addElement(new Byte((byte)0));    
            for (int i = 0; i < willTopic.length; i++)
            {
                construct.addElement(new Byte(willTopic[i]));
            }
            for (int i = 0; i < willMessage.length; i++)
            {
                construct.addElement(new Byte(willMessage[i]));
            }
        }

        // Add User if required
        if (SCADAutils.getUserFlag(connectFlags) == 1)
        {
           for (int i = 0; i < user.length; i++)
            {
                construct.addElement(new Byte(user[i]));
            }
        }

        // Add Password if required
        if (SCADAutils.getUserFlag(connectFlags) == 1)
        {
           for (int i = 0; i < password.length; i++)
            {
                construct.addElement(new Byte(password[i]));
            }
        }
        if (0 != extrabytes) {
        	for (int i=0; i<extrabytes; i++) {
        		construct.addElement(new Byte((byte)1));
        	}
        }

        if (construct.size() != size)
        {
            ScadaException exception = new ScadaException("Size mismatch when constructing msg in function CONNECTmsg buildMsg()");
            throw exception;
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
    public void setFields()
    {

        int offset = 1 + remainingLength.length;

        protocolNameLength = new byte[2];

        protocolNameLength[0] = completeMsg[offset];
        offset++;
        protocolNameLength[1] = completeMsg[offset];
        offset++;

        int nameLength = SCADAutils.UTFLengthAsDecimal(protocolNameLength);

        protocolName = new byte[nameLength];

        for (int i = 0; i < protocolName.length; i++)
        {
            protocolName[i] = completeMsg[offset];
            offset++;
        }

        version = completeMsg[offset];
        offset++;
        connectFlags = completeMsg[offset];
        offset++;

        keepAlive = new byte[2];

        keepAlive[0] = completeMsg[offset];
        offset++;
        keepAlive[1] = completeMsg[offset];
        offset++;

        // Set Client ID
        byte[] lengthTmp = new byte[2];
        lengthTmp[0] = completeMsg[offset];
        offset++;
        lengthTmp[1] = completeMsg[offset];
        offset++;

        int rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);
        rest += 2;
        clientID = new byte[rest];

        clientID[0] = lengthTmp[0];
        clientID[1] = lengthTmp[1];

        for (int i = 2; i < rest; i++)
        {
            clientID[i] = completeMsg[offset];
            offset++;
        }

        // If will stuff is present - set it
        if (SCADAutils.getWillFlag(connectFlags) == 1)
        {

            // Will Topic
            lengthTmp[0] = completeMsg[offset];
            offset++;
            lengthTmp[1] = completeMsg[offset];
            offset++;

            rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);
            rest += 2;
            willTopic = new byte[rest];

            willTopic[0] = lengthTmp[0];
            willTopic[1] = lengthTmp[1];

            for (int i = 2; i < rest; i++)
            {
                willTopic[i] = completeMsg[offset];
                offset++;
            }

            // Will Message
            lengthTmp[0] = completeMsg[offset];
            offset++;
            lengthTmp[1] = completeMsg[offset];
            offset++;

            rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);
            rest += 2;
            willMessage = new byte[rest];

            willMessage[0] = lengthTmp[0];
            willMessage[1] = lengthTmp[1];

            for (int i = 2; i < rest; i++)
            {
                willMessage[i] = completeMsg[offset];
                offset++;
            }

        }

        // If user is present - set it
        if (SCADAutils.getUserFlag(connectFlags) == 1)
        {

            lengthTmp[0] = completeMsg[offset];
            offset++;
            lengthTmp[1] = completeMsg[offset];
            offset++;

            rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);
            rest += 2;
            user = new byte[rest];

            user[0] = lengthTmp[0];
            user[1] = lengthTmp[1];

            for (int i = 2; i < rest; i++)
            {
                user[i] = completeMsg[offset];
                offset++;
            }

        }

        // If password is present - set it
        if (SCADAutils.getPasswordFlag(connectFlags) == 1)
        {

            lengthTmp[0] = completeMsg[offset];
            offset++;
            lengthTmp[1] = completeMsg[offset];
            offset++;

            rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);
            rest += 2;
            password = new byte[rest];

            password[0] = lengthTmp[0];
            password[1] = lengthTmp[1];

            for (int i = 2; i < rest; i++)
            {
                password[i] = completeMsg[offset];
                offset++;
            }

        }

    }

    // Use this function to send a CONNECT msg and have the
    // responding CONNACK handled automatically - if the CONNACK
    // is not what is expected an exception is thrown

    public void sendAndAcknowledge(DataInputStream input, DataOutputStream output, LogFile connectLog, LogFile resultsLog) throws IOException, ScadaException
    {

        CONNACKmsg replyEX = new CONNACKmsg();
        sendAndAcknowledge(input, output, replyEX, connectLog, resultsLog);
    }

    // Use this function to send a CONNECT msg and have the
    // responding CONNACK handled automatically - if the CONNACK
    // is not what is expected an exception is thrown
    public void sendAndAcknowledge(DataInputStream input, DataOutputStream output, CONNACKmsg replyEX, LogFile connectLog, LogFile resultsLog) throws IOException, ScadaException
    {
        send(output, connectLog);

        resultsLog.println("Message sent - waiting for reply...");

        CONNACKmsg reply = new CONNACKmsg(input);
        validateAck(reply, replyEX, connectLog, resultsLog);
    }
    
    
    public void validateAck(CONNACKmsg reply, CONNACKmsg replyEX, LogFile connectLog, LogFile resultsLog) throws IOException, ScadaException
    {
    	connectLog.printTitle("CONNACK from broker:");
        reply.parseAndPrint(connectLog);
        connectLog.printTitle("Expected CONNACK from broker:");
        replyEX.parseAndPrint(connectLog);

        resultsLog.println("CONNACK reply recieved - logged in " + connectLog.name);
        resultsLog.println("Checking reply matches expected reply");

        // Check CONNACK response
        if (SCADAutils.version == 5) {
            if (reply.returnCode != replyEX.returnCode || reply.unused != replyEX.unused) {
                resultsLog.println("CONNACK response did not match expected response");
                throw new ScadaException("Connect failed with: " + SCADAutils.connectReply(reply.returnCode));    
            } else {
                resultsLog.println("Connect succeeded with: " + SCADAutils.connectReply(reply.returnCode));
            }
        } else {    
            if (!SCADAutils.isEqual(reply.completeMsg, replyEX.completeMsg, connectLog))
            {
                resultsLog.println("CONNACK response did not match expected response");
                throw new ScadaException("Connect failed with: " + SCADAutils.connectReply(reply.returnCode));
            }
            else
            {
                resultsLog.println("Connect succeeded with: " + SCADAutils.connectReply(reply.returnCode));
            }
        } 
    }

    // Sets the user and password
    public void setUser(String user) {
    	this.user = SCADAutils.StringToUTF(user);
    	connectFlags = SCADAutils.setUser(connectFlags);
    }

    // Sets the user and password
    public void setPassword(String password) {
    	this.password = SCADAutils.StringToUTF(password);
    	connectFlags = SCADAutils.setPassword(connectFlags);
    }
    
    // Sets the WILL RETAIN flag
    public void setWillRetain() throws ScadaException
    {
        connectFlags = SCADAutils.setWillRetain(connectFlags);
        buildMsg();
    }

    // Modifies the default KEEPALIVE value
    public void setKeepAlive(short secs) throws ScadaException
    {
        keepAlive = SCADAutils.shortToBytes(secs);
        buildMsg();
    }

    // Runs through and prints out message byte information
    public void parseAndPrint(LogFile log) throws IOException
    {
    	if (null != realMsg) {
    		realMsg.parseAndPrint(log);
    	} else {
    		super.parseAndPrint(log);

    		int offset = 1 + remainingLength.length;

    		// Print Protocol name

    		protocolNameLength = new byte[2];

    		SCADAutils.printByteNumeric(completeMsg[offset], log, "PROTOCOL NAME LENGTH(1)", offset);
    		protocolNameLength[0] = completeMsg[offset];
    		offset++;
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "PROTOCOL NAME LENGTH(2)", offset);
    		protocolNameLength[1] = completeMsg[offset];
    		offset++;

    		int nameLength = SCADAutils.UTFLengthAsDecimal(protocolNameLength);

    		for (int i = 0; i < nameLength; i++)
    		{
    			SCADAutils.printByteChar(completeMsg[offset], log, "PROTOCOL NAME", offset);
    			offset++;
    		}

    		// Print Version
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "VERSION", offset);
    		offset++;

    		// Print Connect Flags
    		log.print("BYTE[" + offset + "] =\t");
    		SCADAutils.printBinary(completeMsg[offset], log);
    		log.print("\t\tCONNECT FLAGS: ");
    		log.print("||USER: " + SCADAutils.isUserFlagSet(connectFlags));
    		log.print("||PASSWORD: " + SCADAutils.isPasswordFlagSet(connectFlags));
    		log.print("||WILL RETAIN: " + SCADAutils.isWillRetainFlagSet(connectFlags));
    		log.print("||WILL QOS: " + SCADAutils.getWillQos(connectFlags));
    		log.print("||WILL FLAG: " + SCADAutils.isWillFlagSet(connectFlags));
    		log.print("||CLEAN START: " + SCADAutils.isCleanStartSet(connectFlags));
    		log.newLine();
    		offset++;

    		// Print Keep alive timer
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "KEEP ALIVE(1)", offset);
    		offset++;
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "KEEP ALIVE(2)", offset);
    		offset++;
    		
    		if (SCADAutils.version >= 5) {
        		// Print properties 
        		int proplen = completeMsg[offset]&0xff;
                SCADAutils.printByteNumeric(completeMsg[offset], log, "PROPLEN", offset);
                offset++;
                for (int j=0; j<proplen; j++) {
                    SCADAutils.printByteNumeric(completeMsg[offset], log, "PROPERTIES", offset);
                    offset++;
                }
    		}

    		// Print Client ID
    		byte[] lengthTmp = new byte[2];
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "CLIENT ID LENGTH(1)", offset);
    		lengthTmp[0] = completeMsg[offset];
    		offset++;
    		SCADAutils.printByteNumeric(completeMsg[offset], log, "CLIENT ID LENGTH(2)", offset);
    		lengthTmp[1] = completeMsg[offset];
    		offset++;

    		int rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);

    		for (int i = 0; i < rest; i++)
    		{
    			SCADAutils.printByteChar(completeMsg[offset], log, "CLIENT ID", offset);
    			offset++;
    		}

    		// If will stuff is present - print it
    		if (SCADAutils.getWillFlag(connectFlags) == 1)
    		{
                if (SCADAutils.version >= 5) 
                    offset++;
    			// Will Topic
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "WILL TOPIC LENGTH(1)", offset);
    			lengthTmp[0] = completeMsg[offset];
    			offset++;
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "WILL TOPIC LENGTH(2)", offset);
    			lengthTmp[1] = completeMsg[offset];
    			offset++;

    			rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);

    			for (int i = 0; i < rest; i++)
    			{
    				SCADAutils.printByteChar(completeMsg[offset], log, "WILL TOPIC", offset);
    				offset++;
    			}

    			// Will Message
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "WILL MESSAGE LENGTH(1)", offset);
    			lengthTmp[0] = completeMsg[offset];
    			offset++;
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "WILL MESSAGE LENGTH(2)", offset);
    			lengthTmp[1] = completeMsg[offset];
    			offset++;

    			rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);

    			for (int i = 0; i < rest; i++)
    			{
    				SCADAutils.printByteChar(completeMsg[offset], log, "WILL MESSAGE", offset);
    				offset++;
    			}

    		}

    		// If user stuff is present - print it
    		if (SCADAutils.getUserFlag(connectFlags) == 1)
    		{

    			// User
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "USER LENGTH(1)", offset);
    			lengthTmp[0] = completeMsg[offset];
    			offset++;
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "USER LENGTH(2)", offset);
    			lengthTmp[1] = completeMsg[offset];
    			offset++;

    			rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);

    			for (int i = 0; i < rest; i++)
    			{
    				SCADAutils.printByteChar(completeMsg[offset], log, "USER", offset);
    				offset++;
    			}

    		}

    		// If password stuff is present - print it
    		if (SCADAutils.getPasswordFlag(connectFlags) == 1)
    		{

    			// User
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "PASSWORD LENGTH(1)", offset);
    			lengthTmp[0] = completeMsg[offset];
    			offset++;
    			SCADAutils.printByteNumeric(completeMsg[offset], log, "PASSWORD LENGTH(2)", offset);
    			lengthTmp[1] = completeMsg[offset];
    			offset++;

    			rest = SCADAutils.UTFLengthAsDecimal(lengthTmp);

    			for (int i = 0; i < rest; i++)
    			{
    				SCADAutils.printByteChar(completeMsg[offset], log, "PASSWORD", offset);
    				offset++;
    			}

    		}
    	}
    }

}
