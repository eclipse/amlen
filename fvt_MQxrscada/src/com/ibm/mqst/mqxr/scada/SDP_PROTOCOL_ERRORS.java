package com.ibm.mqst.mqxr.scada;
/*
 * Copyright (c) 2006-2021 Contributors to the Eclipse Foundation
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
import java.io.*;
import java.net.SocketException;
import java.net.SocketTimeoutException;

public class SDP_PROTOCOL_ERRORS
{
    private final static byte INVALID_PROTOCOL_VERSION = 0x04;
    private final static byte UNACCEPTABLE_PROTOCOL_VERSION = 0x01;
    private final static byte INVALID_VERSION_5 = (byte)0x84;
    private final static byte[] INVALID_PROTOCOL = { 'R', 'U', 'B', 'B', 'I', 'S', 'H' }; 
    private final static String LONG_CLIENT_ID = "This id is too long by 1";

    public static void main(String[] args)
    {
        // Testcase
        String testName = "SDP_PROTOCOL_ERRORS";
        String hostName = "localhost";
        boolean testPassed = true;

        int port = 1883;
        String arg;
        int i = 0;    
        boolean testmode = false;

        // Logs
        String resultsLogName = testName + "_RESULTS.log";
        String connectLogName = testName + "_CONNECT.log";
        String pubLogName = testName + "_PUBLISH.log";
        String subLogName = testName + "_SUBSCRIBE.log";
        LogFile resultsLog = null;
        LogFile connectLog = null;
        LogFile subLog = null;
        LogFile pubLog = null;

        // Messages // Expected messages
        CONNECTmsg msg;
        SUBSCRIBEmsg sub;
        PUBLISHmsg pubOut;
        PUBLISHmsg pubIn;
        PUBLISHmsg pubInEX;
        DISCONNECTmsg disMsg;
        CONNACKmsg connackEx;
        CONNACKmsg connack;
        PUBACKmsg pubAck;
        PUBRELmsg pubRel;
        PUBCOMPmsg pubComp;
        PUBCOMPmsg pubCompEx;        

        // Support classes
        Client subscriber;
        Client publisher;
        Client invalidClient;
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;

        // Test data
        String subscriberClientId = "ErrTestSubClient";
        String publisherClientId = "ErrTestPubClient";
       
        String topic1 = "ErrorsTopic";
        String willTopic = "WillErrorsTopic";
        String willMessage = "WillErrorsMessage";
        byte[] payload1 = "This is payload 1".getBytes();
        byte qos0 = 0;
        byte qos1 = 1;
        byte qos2 = 2;
        byte qos3 = 3;

        try
        {

            /** *********************************************************** */
            // SETUP AND CONNECT
            /** *********************************************************** */

            resultsLog = new LogFile(resultsLogName);
            connectLog = new LogFile(connectLogName);
            subLog = new LogFile(subLogName);
            pubLog = new LogFile(pubLogName);
            msgIds = new MsgIdHandler();


            
            // Parse the input parameters.
            while (args != null && i < args.length && args[i].startsWith("-")) {
            	arg = args[i++];
            	if (arg.equals("-hostname") || arg.equals("-h")) {
            		if (i < args.length) {
            			hostName = args[i++];
            		}
            		else {
        	            resultsLog.println("Arguments error: hostname requires a name or ip");            			
            		}
            	}
            	else if (arg.equals("-port") || arg.equals("-p")) {
            		if (i < args.length) {
            			try {
            				port = Integer.parseInt(args[i++]);
            			}
            			catch (NumberFormatException nfe) {
            	            resultsLog.println("Arguments error: port requires a number");            				
            			}
            		}
            	}
            	else if (arg.equals("-version") || arg.equals("-v")) {
            		if (i < args.length) {
            			try {
            				byte version = Byte.parseByte(args[i++]);
            				SCADAutils.setVersion(version);
            			}
            			catch (NumberFormatException nfe) {
            	            resultsLog.println("Arguments error: version requires a number");            				
            			}
            		}
            	}
                else if (arg.equals("-debug") || arg.equals("-d")) {
                    resultsLog = new LogFile("stdout");
                }
                else if (arg.equals("-test") || arg.equals("-t")) {
                    testmode = true;
                }
            } 
            
            resultsLog.printTitle("Starting test " + testName + " version=" + SCADAutils.version);
            System.out.println("Starting test " + testName + " version=" + SCADAutils.version);
            
            // Create socket and setup data streams
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
            publisher = new Client(hostName, port);
            subscriber = new Client(hostName, port);
            resultsLog.println("Connected");

            // Connect to broker and wait for reply
            resultsLog.printTitle("CONNECTING TO BROKER");
            resultsLog.println("Connecting publisher");
            msg = new CONNECTmsg(publisherClientId);
            msg.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), connectLog, resultsLog);

            resultsLog.println("Connecting subscriber");
            msg = new CONNECTmsg(subscriberClientId);
            msg.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), connectLog, resultsLog);

            
            /** *********************************************************** */
            // Subscribe with the QOS level set to 3
            /** *********************************************************** */

            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("SUBSCRIBE WITH INVALID QOS VALUE 3");
                resultsLog.println("Subscribing to " + topic1);
          
                subList = new SubList(topic1, qos3);
                subId = msgIds.getId();
          
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
          
                sub = new SUBSCRIBEmsg(subList, subId);
          
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
          
                sub.send(subscriber.getOutputStream(), subLog);
                // should not get a SUBACK back, but if we do get a message,
                // write the message details to the SUBSCRIBE log file
                resultsLog.println("Waiting to check no SUBACK message is received");
                try
                {
                    SUBACKmsg sachk;
                    sachk = new SUBACKmsg(subscriber.getInputStream());
                    if (SCADAutils.version >= 5) {
                        if (sachk.fixedHeader != (byte)0xE0)
                            throw new ScadaException("Did not get a DISCONNECT");
                    } else {
                        subLog.println( "Unexpected SUBACK msg received from broker" );
                        sachk.parseAndPrint( subLog );
                        throw new ScadaException("Didn't get expected exception");
                    }
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no SUBACK message received as expected");
                }
                catch (EOFException e)
                {
                    resultsLog.println("Expected EOFException received - no SUBACK message received as expected");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
            
            /*** Try closing subscriber in case still open ***/
            try {
            	subscriber.finalize();
            } catch (Throwable t) {}

            /** *********************************************************** */
            // Re-connect subscriber client
            /** *********************************************************** */
            subscriber = new Client(hostName, port);
            resultsLog.println("Subscriber reconnected");

            // Connect to broker and wait for reply
            resultsLog.printTitle("RECONNECTING SUBSCRIBER CLIENT TO BROKER");

            msg = new CONNECTmsg(subscriberClientId);
            msg.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), connectLog, resultsLog);

            
            /** *********************************************************** */
            // Subscribe to a topic and make sure the PUBLISH doesn't arrive when
            // published at QOS3
            /** *********************************************************** */

            try
            {
                resultsLog.printTitle("PUBLISH WITH INVALID QOS VALUE 3");
                resultsLog.println("Subscribing to " + topic1);
     
                subList = new SubList(topic1, qos0);
                subId = msgIds.getId();
     
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
     
                sub = new SUBSCRIBEmsg(subList, subId);
     
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
     
                resultsLog.println("Creating publication message - " + topic1);
                subId = msgIds.getId();
                pubOut = new PUBLISHmsg(topic1, subId, payload1, qos3);
                
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
     
                pubOut.send(publisher.getOutputStream(), pubLog);
                // should not get a PUBACK back
                resultsLog.println("Waiting to check no PUBACK message is received");
                try
                {
                    PUBACKmsg pack = new PUBACKmsg(publisher.getInputStream());
                    if (SCADAutils.version >= 5) {
                        if (pack.fixedHeader != (byte)0xE0)
                            throw new ScadaException("Did not get a DISCONNECT");
                    } else {
                        subLog.println( "Unexpected msg received from broker" );
                        pack.parseAndPrint( pubLog );
                        throw new ScadaException("Didn't get expected exception");
                    }    
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no PUBACK message received as expected");
                }
                catch (EOFException e)
                {
                    resultsLog.println("Expected EOFException received - no PUBACK message received as expected");
                }
                // subscriber should not get a PUBLISH but if it does get a message,
                // write the message details to the PUBLISH log file
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    PUBLISHmsg pubchk;
                    pubchk = new PUBLISHmsg(subscriber.getInputStream());
                    pubLog.printTitle( "Unexpected PUBLISH msg received from broker" );
                    pubchk.parseAndPrint( pubLog );
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no PUBLISH message received as expected");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
            
            
            /*** Try closing publisher in case still open ***/
            try {
            	publisher.finalize();
            } catch (Throwable t) {}
            /** *********************************************************** */
            // Re-connect publisher client
            /** *********************************************************** */
            publisher = new Client(hostName, port);
            resultsLog.println("Publisher reconnected");

            // Connect to broker and wait for reply
            resultsLog.printTitle("RECONNECTING PUBLISHER CLIENT TO BROKER");

            msg = new CONNECTmsg(publisherClientId);
            msg.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), connectLog, resultsLog);            
            
            /** *********************************************************** */
            // Connect using invalid protocol name 
            /** *********************************************************** */

            resultsLog.printTitle("CONNECTING TO BROKER USING INVALID PROTOCOL NAME");
            // Create socket and setup data streams
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
            invalidClient = new Client(hostName, port);
            resultsLog.println("Connected");

            // Connect to broker - should get disconnected
            resultsLog.println("Sending connect message");
            msg = new CONNECTmsg(invalidClient.getClientId(), true, INVALID_PROTOCOL, SCADAutils.version);
            try
            {
            	CONNACKmsg connackEx2 = new CONNACKmsg(SCADAutils.version >= 5 ? INVALID_VERSION_5 : UNACCEPTABLE_PROTOCOL_VERSION);
                msg.sendAndAcknowledge(invalidClient.getInputStream(), invalidClient.getOutputStream(), connackEx2, connectLog, resultsLog);

            }
            // either SocketException or EOFException is acceptable here
            catch (SocketException e)
            {
                resultsLog.println("Expected SocketException received - DISCONNECT was recognised");
            }
            catch (EOFException e)
            {
                resultsLog.println("Expected EOFException received - DISCONNECT was recognised");
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
    
            /** *********************************************************** */
            // Connect using invalid protocol version
            /** *********************************************************** */

            
            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            
            resultsLog.printTitle("CONNECTING TO BROKER USING INVALID PROTOCOL VERSION");
            // Create socket and setup data streams
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
            invalidClient = new Client(hostName, port);
            resultsLog.println("Connected");

            // Connect to broker and wait for reply
            resultsLog.println("Sending connect message");
            msg = new CONNECTmsg(invalidClient.getClientId(), true, SCADAutils.protocolName, (byte)(SCADAutils.getVersion()+1));
            try
            {
            	CONNACKmsg connackEx2 = new CONNACKmsg(SCADAutils.version >= 5 ? INVALID_VERSION_5 : UNACCEPTABLE_PROTOCOL_VERSION);
                msg.sendAndAcknowledge(invalidClient.getInputStream(), invalidClient.getOutputStream(), connackEx2, connectLog, resultsLog);

            }
            // either SocketException or EOFException is acceptable here
            catch (SocketException e)
            {
                resultsLog.println("Expected SocketException received - DISCONNECT was recognised");
            }
            catch (EOFException e)
            {
                resultsLog.println("Expected EOFException received - DISCONNECT was recognised");
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
               

            /** *********************************************************** */
            // Send a CONNECT message missing a client identifier:
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("CONNECTING TO BROKER USING MISSING CLIENT IDENTIFIER");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
    
                // Connect to broker and wait for reply
                resultsLog.println("Sending connect message");
                msg = new CONNECTmsg("", true, SCADAutils.protocolName, SCADAutils.version);
                msg.send(invalidClient.getOutputStream(), connectLog);
    
                // should get a CONNACK back with return code 2- identifier rejected
                if (SCADAutils.getVersion() >= 4) {
                	connackEx = new CONNACKmsg();
                } else {
                    connackEx = new CONNACKmsg((byte) 2);
                }
                connack = new CONNACKmsg(invalidClient.getInputStream());
                SCADAutils.compareAndLog(connack, connackEx, connectLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // Send a message with a client identifier > 23 characters
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("CONNECTING TO BROKER USING CLIENT IDENTIFIER > 23 CHARACTERS");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
    
                // Connect to broker and wait for reply
                resultsLog.println("Sending connect message");
                msg = new CONNECTmsg(LONG_CLIENT_ID, true, SCADAutils.protocolName, SCADAutils.version);
                msg.send(invalidClient.getOutputStream(), connectLog);
    
                // should get a CONNACK back with return code 2- identifier rejected
                // No longer, should always succeed
                //if (4 == SCADAutils.getVersion()) {
                	connackEx = new CONNACKmsg();
                //} else {
                //    connackEx = new CONNACKmsg((byte) 2);
                //}
                connack = new CONNACKmsg(invalidClient.getInputStream());
                SCADAutils.compareAndLog(connack, connackEx, connectLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            
            /** *********************************************************** */
            // Send rubbish data to the broker port- should get disconnected
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("SEND RUBBISH DATA TO THE SCADA PORT");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                try
                {
                    invalidClient.getOutputStream().writeBytes("Sending this to the broker should get us disconnected");
                    invalidClient.getOutputStream().flush();
                    // ensure the bytes are written to the wire by forcing a line turn around! 
                    invalidClient.getInputStream().readByte();
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketException e)
                {
                    resultsLog.println("Disconnected as expected (SocketException)");
                }
                catch (EOFException e)
                {
                    resultsLog.println("Disconnected as expected (EOFException)");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
            
            
            /** *********************************************************** */
            // Send a WILL connect message which is missing a WILL message
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("SEND A WILL CONNECT MESSAGE WHICH IS MISSING A WILL MESSAGE");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                // Connect to broker and wait for reply
                resultsLog.println("Sending connect message");
                msg = new CONNECTmsg("EmptyWillMessage", willTopic, "", qos1);

                // A zero-length Will message is acceptable so the CONNECT message
                // should be acknowledged with return code 0 in he CONNACK
                msg.sendAndAcknowledge( invalidClient.getInputStream(), 
                                        invalidClient.getOutputStream(), 
                                        connectLog, resultsLog);

            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
            
            
            
            /** *********************************************************** */
            // Send a WILL connect message which has a WILL QoS of 3
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("SEND A WILL CONNECT MESSAGE WHICH HAS A WILL QOS OF 3");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                // Connect to broker and wait for reply
                resultsLog.println("Sending connect message");
                msg = new CONNECTmsg(invalidClient.getClientId(), willTopic, willMessage, qos3);

                // A Will QoS of 3 should result in a termination of the connection
                // should be acknowledged with return code 0 in he CONNACK
                msg.sendAndAcknowledge( invalidClient.getInputStream(), 
                                        invalidClient.getOutputStream(), 
                                        connectLog, resultsLog);
                throw new ScadaException("Didn't get expected exception");
            }
            catch (SocketTimeoutException e)
            {
                resultsLog.println("Disconnected as expected (SocketException)");
            }
            catch (EOFException e) {
            	resultsLog.println("Disconnected as expected (EOFException)");
            }
            catch (SocketException e) {
            	resultsLog.println("Disconnected as expected (SocketException)");
            }
            
            
            /** *********************************************************** */
            /* Send subscribe/unsubscribe with a QOS of 0 or 2
             *
             * Leaving the code for this test in incase the spec should change.
             * Currently a SUBSCRIBE message is treated as a QOS1 message
             * regardless of what the QOS value is set to as it has to be a
             * QOS1 message
             */
            /** *********************************************************** */
/*
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("SEND SUBSCRIBE MESSAGE WITH QOS SET TO 0");
    
                subList = new SubList(topic1, qos0);
                subId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, subId, qos0);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.send(subscriber.getOutputStream(), subLog);
                // should not get a SUBACK back
                resultsLog.println("Waiting to check no SUBACK message is received");
                try
                {
                    subAck = new SUBACKmsg(subscriber.getInputStream());
                    subAckEx = new SUBACKmsg(new SubList("wibble", (byte) 1),subId);
                    SCADAutils.compareAndLog(subAck, subAckEx, connectLog, resultsLog);
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no SUBACK message received as expected");
                }
    
                
                resultsLog.printTitle("SEND SUBSCRIBE MESSAGE WITH QOS SET TO 2");
                subList = new SubList(topic1, qos0);
                subId = msgIds.getId();
                sub = new SUBSCRIBEmsg(subList, subId, qos2);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.send(subscriber.getOutputStream(), subLog);
                // should not get a SUBACK back
                resultsLog.println("Waiting to check no SUBACK message is received");
                try
                {
                    subAck = new SUBACKmsg(subscriber.getInputStream());
                    subAckEx = new SUBACKmsg(new SubList("wibble", (byte) 1), subId);
                    SCADAutils.compareAndLog(subAck, subAckEx, connectLog, resultsLog);
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no SUBACK message received as expected");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
*/
            
            /** *********************************************************** */
            // Send erroneous response messages: PUBACK, PUBREL, PUBCOMP
            /** *********************************************************** */

            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("SEND ERRONEOUS PUBACK, PUBREL AND SUBACK MESSAGES");
                
                // Create subscription list and subscribe
                resultsLog.println("Subscribing to a topic");
                subList = new SubList(topic1, qos0);
                subId = msgIds.getId();
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscripiton message");
                sub = new SUBSCRIBEmsg(subList, subId);
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // MQXR ignores unsolicited/duplicate PUBACKs 
                resultsLog.println("Sending PUBACK message to broker...");
                subId = msgIds.getId();
                pubAck = new PUBACKmsg(subId);
                pubAck.send(subscriber.getOutputStream(), subLog);
                
                // MQXR accepts unsolicited/duplicate PUBRELs and responds
                // with a PUBCOMP. 
                resultsLog.println("Sending PUBREL message to broker...");
                subId = msgIds.getId();
                pubRel = new PUBRELmsg(subId);
                pubRel.send(subscriber.getOutputStream(), subLog);
                if (SCADAutils.version >= 5)
                    pubCompEx = new PUBCOMPmsg(subId, 146);
                else
                    pubCompEx = new PUBCOMPmsg( subId );
                pubComp = new PUBCOMPmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubComp, pubCompEx, subLog, resultsLog);
                
                // MQXR treats an unsolicited PUBCOMP as a protocol error and
                // generates an FDC to indicate that it has no saved state
                resultsLog.println("Sending PUBCOMP message to broker...");
                subId = msgIds.getId();
                pubComp = new PUBCOMPmsg(subId);
                pubComp.send(subscriber.getOutputStream(), subLog);
                
                resultsLog.println("Publishing a message to check flow is still running...");
                resultsLog.println("Creating publication message - QoS 0");
                pubOut = new PUBLISHmsg(topic1, payload1);
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, pubLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */
            // Check that socket will close if no activity within one minute:
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("CONNECTING TO BROKER NO CONNECT MSG FOR 60 SECONDS");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                // We will wait 61 seconds to make sure.
                long endTime = System.currentTimeMillis() + (1000*61);
                while (System.currentTimeMillis() < endTime) {
                	Thread.sleep(endTime - System.currentTimeMillis());
                }
                if (!invalidClient.isSocketConnected()) {
                	resultsLog.println("Socket is not connected.");
                	testPassed = false;
                }
    
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // Check that socket will close if CONNECT is not successful within one minute:
            /** *********************************************************** */
/*
            try
            {
                resultsLog.printTitle("CONNECTING TO BROKER NO SUCCESSFUL CONNECT MSG FOR 60 SECONDS");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                // We will wait 61 seconds to make sure.
                long endTime = System.currentTimeMillis() + (1000*60) - 300;
                while (System.currentTimeMillis() < endTime) {
                    // Connect to broker and wait for reply
                    resultsLog.println("Sending connect message");
                    msg = new CONNECTmsg("", true, SCADAutils.protocolName, SCADAutils.version);
                    msg.send(invalidClient.getOutputStream(), connectLog);
        
                    // should get a CONNACK back with return code 2- identifier rejected
                    //connackEx = new CONNACKmsg((byte) 2);
                    connack = new CONNACKmsg(invalidClient.getInputStream());
                    //SCADAutils.compareAndLog(connack, connackEx, connectLog, resultsLog);

                    Thread.sleep(1000);
                }
                // Sleep one extra second to ensure we are passed the 60 seconds.
                Thread.sleep(1000);
                if (!invalidClient.isSocketClosed()) {
                	resultsLog.println("Socket is not closed.");
                	testPassed = false;
                }
    
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
*/

            /** *********************************************************** */
            // Check that socket will close if first message is not CONNECT:
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                resultsLog.printTitle("CONNECTING TO BROKER FIRST MSG NOT CONNECT");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");

                // Connect to broker and wait for reply
                resultsLog.println("Sending connect message");
                subList = new SubList(topic1, qos3);
                subId = msgIds.getId();

                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");

                sub = new SUBSCRIBEmsg(subList, subId);
                resultsLog.println("Before send invalidClient.isSocketClosed()="+invalidClient.isSocketClosed());
                sub.send(invalidClient.getOutputStream(), connectLog);

                try
                {
                    SUBACKmsg sachk;
                    sachk = new SUBACKmsg(invalidClient.getInputStream());
                    subLog.printTitle( "Unexpected SUBACK msg received from broker" );
                    sachk.parseAndPrint( subLog );
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no SUBACK message received as expected");
                }
                catch (EOFException e)
                {
                    resultsLog.println("Expected EOFException received - no SUBACK message received as expected");
                }

                resultsLog.println("Well after send invalidClient.isSocketClosed()="+invalidClient.isSocketClosed());
                /*if (invalidClient.isSocketClosed()) {
                	resultsLog.println("Socket is not connected.");
                	testPassed = false;
                }*/
    
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            // CONNECT messages are now accepted up to about 64K*5
            /** *********************************************************** */
            // Check that socket will close if CONNECT msg is larger than 64K:
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            try
            {
                String user;
                String password;
                String longWillTopic;
                String longWillMessage;

                resultsLog.printTitle("CONNECTING TO BROKER VERY LARGE CONNECT MSG");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                int size = 22000;
                do {
                    longWillTopic = SCADAutils.randomString(size, 30);
                    size -= 100;
                } while (longWillTopic.getBytes("UTF-8").length > 64000);
                resultsLog.println("Created longWillTopic length "+longWillTopic.getBytes("UTF-8").length);
                size = 22000;
                do {
                    longWillMessage = SCADAutils.randomString(size, 30);
                    size -= 100;
                } while (longWillMessage.getBytes().length > 64000);
                resultsLog.println("Created longWillMessage length "+longWillMessage.getBytes("UTF-8").length);
                msg = new CONNECTmsg(invalidClient.getClientId(), longWillTopic, longWillMessage, qos3, 1234567);
                size = 18000;
                do {
                    user = SCADAutils.randomString(size);
                    size -= 100;
                    if (user.getBytes().length > 64000) {
                        resultsLog.println("Created user length "+user.length()+" bytes length is "+user.getBytes("UTF-8").length);
                    }
                } while (user.getBytes().length > 64000);
                resultsLog.println("Created user length "+user.getBytes("UTF-8").length);
                msg.setUser(user);
                size = 18000;
                do {
                    password = SCADAutils.randomString(size);
                    size -= 100;
                    if (password.getBytes().length > 64000) {
                        resultsLog.println("Created password length "+password.length()+" bytes length is "+password.getBytes("UTF-8").length);
                    }
                } while (password.getBytes().length > 64000);
                msg.setPassword(password);
                resultsLog.println("Created password length "+password.getBytes("UTF-8").length);
                try {
                	msg.send(invalidClient.getOutputStream(), connectLog);
                	// Sleep one extra second to ensure we are passed the 60 seconds.
                    resultsLog.println("Expected SocketException not received");
                    testPassed = false;
                } catch (SocketException se) {
                    resultsLog.println("Expected SocketException received");
                }
    
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            // CONNECT messages with invalid flag bits should close the connection
            /** *********************************************************** */
            // Check that socket will close if CONNECT msg has invalid flag bits:
            // Only if 3.1.1
            /** *********************************************************** */

            /*** Try closing invalidClient in case still open ***/
            try {
            	invalidClient.finalize();
            } catch (Throwable t) {}
            if (SCADAutils.version >= SCADAutils.version311)
            try
            {
                resultsLog.printTitle("CONNECTING TO BROKER Invalid bit 0");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                msg = new CONNECTmsg(invalidClient.getClientId());
                msg.fixedHeader |= 1;
                try {
                	msg.send(invalidClient.getOutputStream(), connectLog);
                    SUBACKmsg sachk;
                    sachk = new SUBACKmsg(subscriber.getInputStream());
                    // Should not get SUBACK back
                    resultsLog.println("Expected SocketException not received");
                    testPassed = false;
                } catch (SocketException se) {
                    resultsLog.println("Expected SocketException received");
                } catch (SocketTimeoutException se) {
                	if (invalidClient.isSocketClosed()) {
                		resultsLog.println("Received SocketTimeoutException, socket says not closed.");
                		testPassed = false;
                	}
                }
                try {
                	invalidClient.finalize();
                } catch (Throwable t) {}

                resultsLog.printTitle("CONNECTING TO BROKER Invalid bit 1");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                msg = new CONNECTmsg(invalidClient.getClientId());
                msg.fixedHeader |= 2;
                try {
                	msg.send(invalidClient.getOutputStream(), connectLog);
                    SUBACKmsg sachk;
                    sachk = new SUBACKmsg(subscriber.getInputStream());
                    // Should not get SUBACK back
                    resultsLog.println("Expected SocketException not received");
                    testPassed = false;
                } catch (SocketException se) {
                    resultsLog.println("Expected SocketException received");
                } catch (SocketTimeoutException se) {
                	if (invalidClient.isSocketClosed()) {
                		resultsLog.println("Received SocketTimeoutException, socket says not closed.");
                		testPassed = false;
                	}
                }
                try {
                	invalidClient.finalize();
                } catch (Throwable t) {}

                resultsLog.printTitle("CONNECTING TO BROKER Invalid bit 2");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                msg = new CONNECTmsg(invalidClient.getClientId());
                msg.fixedHeader |= 4;
                try {
                	msg.send(invalidClient.getOutputStream(), connectLog);
                    SUBACKmsg sachk;
                    sachk = new SUBACKmsg(subscriber.getInputStream());
                    // Should not get SUBACK back
                    resultsLog.println("Expected SocketException not received");
                    testPassed = false;
                } catch (SocketException se) {
                    resultsLog.println("Expected SocketException received");
                } catch (SocketTimeoutException se) {
                	if (invalidClient.isSocketClosed()) {
                		resultsLog.println("Received SocketTimeoutException, socket says not closed.");
                		testPassed = false;
                	}
                }
                try {
                	invalidClient.finalize();
                } catch (Throwable t) {}

                resultsLog.printTitle("CONNECTING TO BROKER Invalid bit 3");
                // Create socket and setup data streams
                resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
                invalidClient = new Client(hostName, port);
                resultsLog.println("Connected");
                
                msg = new CONNECTmsg(invalidClient.getClientId());
                msg.fixedHeader |= 8;
                try {
                	msg.send(invalidClient.getOutputStream(), connectLog);
                    SUBACKmsg sachk;
                    sachk = new SUBACKmsg(subscriber.getInputStream());
                    // Should not get SUBACK back
                    resultsLog.println("Expected SocketException not received");
                    testPassed = false;
                } catch (SocketException se) {
                    resultsLog.println("Expected SocketException received");
                } catch (SocketTimeoutException se) {
                	if (invalidClient.isSocketClosed()) {
                		resultsLog.println("Received SocketTimeoutException, socket says not closed.");
                		testPassed = false;
                	}
                }
                try {
                	invalidClient.finalize();
                } catch (Throwable t) {}
    
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */
            // DISCONNECT msgs
            /** *********************************************************** */
            resultsLog.printTitle("DISCONNECT MESSAGES");
            resultsLog.println("Preparing DISCONNECT msg");

            disMsg = new DISCONNECTmsg();

            resultsLog.println("DISCONNECT message created in " + connectLog.name);
            resultsLog.println("Sending DISCONNECT message to broker...");

            disMsg.send(publisher.getOutputStream(), connectLog);
            disMsg.send(subscriber.getOutputStream(), connectLog);

            resultsLog.printTitle("END OF TEST");
        }
        catch (Exception e)
        {
            testPassed = false;
            e.printStackTrace(resultsLog.out);
        }
        finally
        {
            resultsLog.println(testName + (testPassed ? " PASSED" : " FAILED"));
            System.out.println(testName + (testPassed ? " PASSED" : " FAILED"));
            if (!testmode && testPassed) {
                connectLog.delete();
                subLog.delete();
                pubLog.delete();
            }
            System.exit(testPassed ? 0 : 1 );
        }

        /** *********************************************************** */
        // END TEST
        /** *********************************************************** */
    }
}
