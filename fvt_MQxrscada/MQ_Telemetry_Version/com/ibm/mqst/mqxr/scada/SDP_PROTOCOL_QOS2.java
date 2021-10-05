package com.ibm.mqst.mqxr.scada;
/*
 * Copyright (c) 2001-2021 Contributors to the Eclipse Foundation
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
import java.net.*;

public class SDP_PROTOCOL_QOS2
{

    public static void main(String[] args)
    {
        // Testcase
        String testName = "SDP_PROTOCOL_QOS2";
        String hostName = "localhost";
        boolean testPassed = true;
        int port = 1883;
        String arg;
        int i = 0;        

        if (args.length > 0)
        {
            port = Integer.parseInt(args[0]);
        }
        if ((args.length > 1) && (args[1].toLowerCase().equals("xa")))
        {
            testName = testName + "_XA";
        }

        // Logs
        String resultsLogName = testName + "_RESULTS.log";
        String connectLogName = testName + "_CONNECT.log";
        String pubLogName = testName + "_PUBLISH.log";
        String subLogName = testName + "_SUBSCRIBE.log";
        LogFile resultsLog = null;
        LogFile connectLog = null;
        LogFile subLog = null;
        LogFile pubLog = null;

        // Socket variables
        Socket subscribeSocket;
        Socket publishSocket;
        DataOutputStream subscribeOutput;
        DataOutputStream publishOutput;
        DataInputStream subscribeInput;
        DataInputStream publishInput;

        // Messages // Expected messages
        CONNECTmsg publishConnectMessage;
        CONNECTmsg subscribeConnectMessage;
        SUBSCRIBEmsg sub;

        PUBLISHmsg pubOut1; // Qos 0
        PUBLISHmsg pubOut2; // Qos 1
        PUBLISHmsg pubOut3; // Qos 1 - ack ignored
        PUBLISHmsg pubOut5; // Qos 2 - normal
        PUBLISHmsg pubOut6; // Qos 2 - PUBREC ignored
        PUBLISHmsg pubOut7;
        PUBLISHmsg pubOut8; // Qos 2 - PUBCOMP ignored
        PUBLISHmsg pubOut9; // Qos 2 - PUBREC not sent on get
        PUBLISHmsg pubOut10; // Qos 2 - PUBCOMP not sent on get

        PUBLISHmsg pubIn1;
        PUBLISHmsg pubIn1EX; // Qos 0
        PUBLISHmsg pubIn2;
        PUBLISHmsg pubIn2EX; // Qos 1
        PUBLISHmsg pubIn3;
        PUBLISHmsg pubIn3EX; // Qos 1 - no ack
        PUBLISHmsg pubIn4;
        PUBLISHmsg pubIn4EX; // Qos 1 - no ack
        PUBLISHmsg pubIn5;
        PUBLISHmsg pubIn5EX; // Qos 2 - normal
        PUBLISHmsg pubIn6;
        PUBLISHmsg pubIn6EX; // Qos 2 - PUBREC on pub put ignored
        PUBLISHmsg pubIn7;
        PUBLISHmsg pubIn8;
        PUBLISHmsg pubIn8EX; // Qos 2 - PUBCOMP on pub put ignored
        PUBLISHmsg pubIn9;
        PUBLISHmsg pubIn9EX; // Qos 2 - no PUBREC sent on get
        PUBLISHmsg pubIn10;
        PUBLISHmsg pubIn10EX; // Qos 2 - no PUBCOMP sent on get

        PUBRECmsg pubRec;
        PUBRECmsg pubRecEX;
        PUBRELmsg pubRel;
        PUBRELmsg pubRelEX;
        PUBCOMPmsg pubComp;
        PUBCOMPmsg pubCompEX;

        DISCONNECTmsg disMsg;

        // Support classes
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;
        MsgIdentifier pub2Id; // Qos 1 - normal
        MsgIdentifier pub3Id; // Qos 1 - ack ignored
        MsgIdentifier pub5Id; // Qos 2 - normal
        MsgIdentifier pub6Id; // Qos 2 - PUBREC ack ignored on put PUBLISH
        MsgIdentifier pub7Id; // Qos 2 - reserved ID
        MsgIdentifier pub8Id; // Qos 2 - PUBCOMP ignored
        MsgIdentifier pub9Id; // Qos 2 - no PUBREC sent
        MsgIdentifier pub10Id; // Qos 2 - no PUBCOMP sent

        // Test data
        String publishClientId = "C1QoS2Pub";
        String subscribeClientId = "C2QoS2Sub";
         
        String topic = "SamQos2";
        byte[] payload1 = "Normal QoS0 Message".getBytes();
        byte[] payload2 = "Normal QoS1 Message".getBytes();
        byte[] payload3 = "QoS1 - PUBACK not sent Message".getBytes();
        byte[] payload5 = "Normal QoS2 Message".getBytes();
        byte[] payload6 = "QoS2 - PUBREC ignored on put Message".getBytes();
        byte[] payload7 = "QoS2 - Illegal MsgID Mesasge".getBytes();
        byte[] payload8 = "QoS2 - PUBCOMP ignored Message".getBytes();
        byte[] payload9 = "QoS2 - PUBREC not sent on get Message".getBytes();
        byte[] payload10 = "QoS2 - PUBCOMP not sent on get Message".getBytes();
        byte subQos = 2;
        byte pub2Qos = 1;
        byte pub3Qos = 1; // QoS 1 - ack ignored
        byte pub5Qos = 2; // QoS 2 - normal
        byte pub6Qos = 2; // QoS 2
        byte pub7Qos = 2; // QoS 2
        byte pub8Qos = 2; // QoS 2
        byte pub9Qos = 2; // QoS 2
        byte pub10Qos = 2; // QoS 2

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
            resultsLog.printTitle("Starting test " + testName);
            
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
            }                     
            
            // Create socket and setup data streams

            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);

            subscribeSocket = new Socket(hostName, port);
            publishSocket = new Socket(hostName, port);

            resultsLog.println("Connected");

            subscribeSocket.setSoTimeout(SCADAutils.timeout);
            publishSocket.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            subscribeOutput = new DataOutputStream(subscribeSocket.getOutputStream());
            subscribeInput = new DataInputStream(subscribeSocket.getInputStream());
            publishOutput = new DataOutputStream(publishSocket.getOutputStream());
            publishInput = new DataInputStream(publishSocket.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER");
            resultsLog.println("Creating CONNECT message");

            subscribeConnectMessage = new CONNECTmsg(subscribeClientId,false);
            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT mesasge to broker for subscribe");
            subscribeConnectMessage.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);

            publishConnectMessage = new CONNECTmsg(publishClientId,false);
            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT mesasge to broker for publish");
            publishConnectMessage.sendAndAcknowledge(publishInput, publishOutput, connectLog, resultsLog);

            /** *********************************************************** */
            // SUBSCRIBE
            /** *********************************************************** */
            // Create subscription list and subscribe
            resultsLog.printTitle("SUBSCRIPTIONS");
            resultsLog.println("Creating subscription list");

            subList = new SubList(topic, subQos);

            subId = msgIds.getId();

            resultsLog.println("Subscriptions prepared");
            resultsLog.println("Creating subscripiton message");

            sub = new SUBSCRIBEmsg(subList, subId);

            resultsLog.println("SUBSCRIBE message created in " + subLog.name);
            resultsLog.println("Sending SUBSCRIBE message to broker...");

            sub.sendAndAcknowledge(subscribeInput, subscribeOutput, subLog, resultsLog);

            /** *********************************************************** */
            // PUBLISH MSG TESTING
            /** *********************************************************** */

            // PUBLISH msg (Qos0)
            /*
             * We need to test that a msg published at QoS0 is actually
             * published at QoS0 and not "upgraded"
             */
            try
            {
                // Create publish data and publish
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos0)");
                resultsLog.println("Creating publication message - QoS 0");
    
                pubOut1 = new PUBLISHmsg(topic, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut1.send(publishOutput, pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn1EX = new PUBLISHmsg(topic, payload1);
                pubIn1 = new PUBLISHmsg(subscribeInput);
    
                SCADAutils.compareAndLog(pubIn1, pubIn1EX, pubLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msg (Qos1)
            /*
             * We need to test that a msg published at QoS1 is actually
             * published at QoS1 and not "upgraded"
             */
            try
            {
                // Create publish data and publish - normal acknowledgments
                pub2Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos1) - normal behaviour");
                resultsLog.println("Creating publication message - QoS 1 - normal acknowledgments");
    
                pubOut2 = new PUBLISHmsg(topic, pub2Id, payload2, pub2Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut2.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn2EX = new PUBLISHmsg(topic, pub2Id, payload2, pub2Qos);
                pubIn2 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn2EX);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msg (Qos1)
            /*
             * We need to test that a msg is resent if we dont send a PUBACK
             * when getting a pub at QoS1
             */
            try
            {
                // Create publish data and publish - ignored acknowledgments
                pub3Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos1) - PUBACK ignored");
                resultsLog.println("Creating publication message - QoS 1");
    
                pubOut3 = new PUBLISHmsg(topic, pub3Id, payload2, pub3Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut3.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get a PUBLISH msg back - we'll get it but NOT send a
                // PUBACK

                resultsLog.println("Waiting for PUBLISHED data...");
    
                pubIn3EX = new PUBLISHmsg(topic, pub3Id, payload2, pub3Qos);
                pubIn3 = new PUBLISHmsg(subscribeInput);
                pubIn3EX.setId(pubIn3.myId);
    
                SCADAutils.compareAndLog(pubIn3, pubIn3EX, pubLog, resultsLog);
    
                resultsLog.println("Not sending PUBACK - waiting to see if broker tries resend");

                // Drop the connection, wait then reconnect 
                // The QoS2 message should be resent
                subscribeSocket.close();
                Thread.sleep(5000);

                subscribeSocket = new Socket(hostName, port);

                resultsLog.println("Subscribe client reconnected");

                subscribeSocket.setSoTimeout(SCADAutils.timeout);

                resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

                subscribeOutput = new DataOutputStream(subscribeSocket.getOutputStream());
                subscribeInput = new DataInputStream(subscribeSocket.getInputStream());

                // Connect to broker and wait for reply

                resultsLog.printTitle("RECONNECTING TO BROKER");
                resultsLog.println("Creating CONNECT message");

                subscribeConnectMessage = new CONNECTmsg(subscribeClientId, false);
                resultsLog.println("Message created in " + connectLog.name);
                resultsLog.println("Sending CONNECT mesasge to broker for subscribe");
                subscribeConnectMessage.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);

                resultsLog.println("Waiting for published data - will acknowledge this one");
    
                pubIn4EX = new PUBLISHmsg(topic, pub3Id, payload2, pub3Qos);
                pubIn4EX.setDUP();
                pubIn4 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn4EX);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */
            // REMAKE SUBSCRIPTIONS
            /** *********************************************************** */
            // Create subscription list and subscribe
            resultsLog.printTitle("REMAKE SUBSCRIPTIONS");
            resultsLog.println("Creating subscription list");

            subList = new SubList(topic, subQos);

            subId = msgIds.getId();

            resultsLog.println("Subscriptions prepared");
            resultsLog.println("Creating subscripiton message");

            sub = new SUBSCRIBEmsg(subList, subId);

            resultsLog.println("SUBSCRIBE message created in " + subLog.name);
            resultsLog.println("Sending SUBSCRIBE message to broker...");

            sub.sendAndAcknowledge(subscribeInput, subscribeOutput, subLog, resultsLog);                

            /** *********************************************************** */

            // PUBLISH msg (Qos2)
            /*
             * We need to check QoS2 msg flow behaves as described in the SCADA
             * spec under normal conditions (i.e. all acks done)
             */
            try
            {
                // Create publish data and publish
                pub5Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2) - normal");
                resultsLog.println("Creating publication message - QoS 2");
    
                pubOut5 = new PUBLISHmsg(topic, pub5Id, payload5, pub5Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut5.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn5EX = new PUBLISHmsg(topic, pub5Id, payload5, pub5Qos);
                pubIn5 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn5EX);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msg (Qos2)
            /*
             * We need to check QoS2 msg flow behaves as described in the SCADA
             * spec if we ignore the PUBREC response and resend the original msg
             * with the DUP flag set - we should still only get one PUBLISH back
             * under our subscribed topic
             */
            try
            {
                // Create publish data and publish
                pub6Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2) - PUBREC ignored");
                resultsLog.println("Creating publication message - QoS 2");
    
                pubOut6 = new PUBLISHmsg(topic, pub6Id, payload6, pub6Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut6.send(publishOutput, pubLog);
    
                resultsLog.println("Waiting for PUBREC reply...");
    
                pubRec = new PUBRECmsg(publishInput);
                pubRecEX = new PUBRECmsg(pub6Id);
    
                SCADAutils.compareAndLog(pubRec, pubRecEX, pubLog, resultsLog);
                resultsLog.println("PUBREC received - ignoring and resending original msg with DUP flag set");
    
                pubOut6.setDUP();
                pubOut6.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Now we should get back ONE publish msg in response:
    
                resultsLog.println("Waiting for published data");
    
                pubIn6EX = new PUBLISHmsg(topic, pub6Id, payload6, pub6Qos);
                pubIn6 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn6EX);
    
                try
                {
                    pubIn6 = new PUBLISHmsg(subscribeInput);
                    throw new ScadaException("Received a PUBLISH message when one should not have been available");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("No PUBLISH message available - good");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msg (Qos2)
            /*
             * We need to check a QoS2 msg with a reserved id (0) is treated as
             * illegal
             */
/*
            try
            {
                // Create publish data and publish
                
                pub7Id = new MsgIdentifier(0);
                
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2) - reserved ID"); 
                resultsLog.println("Creating publication message - QoS 2 - reserved ID");
                
                pubOut7 = new PUBLISHmsg(topic, pub7Id, payload7, pub7Qos);
                
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
                
                pubOut7.send(publishOutput, pubLog);
                
                // Should NOT get a PUBLISH msg back
                try
                {
                    pubIn7 = new PUBLISHmsg(subscribeInput);
                    throw new ScadaException("Received a PUBLISH message when one should not have been available");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("No PUBLISH message available - good");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */
            /** *********************************************************** */

            // PUBLISH msg (Qos2)
            /*
             * We need to check QoS2 msg flow behaves as described in the SCADA
             * spec if we ignore the PUBCOMP response and resend the PUBREL msg -
             * we should still only get one PUBLISH back under our subscribed
             * topic
             */
            try
            {
                // Create publish data and publish
                pub8Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2) - PUBCOMP ignored");
                resultsLog.println("Creating publication message - QoS 2");
    
                pubOut8 = new PUBLISHmsg(topic, pub8Id, payload8, pub8Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut8.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Now we should get back ONE publish msg in response:
    
                resultsLog.println("Waiting for published data");
    
                pubIn8EX = new PUBLISHmsg(topic, pub8Id, payload8, pub8Qos);
                pubIn8 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn8EX);
    
                resultsLog.println("We will now pretend we never got a PUBCOMP and resend the PUBREL msg - we should get a new PUBCOMP but no PUBLISH msg should be received");
    
                pubRel = new PUBRELmsg(pubOut8.myId);
    
                resultsLog.println("PUBREL message created in " + pubLog.name);
                resultsLog.println("Sending PUBREL message to broker...");
    
                pubRel.send(publishOutput, pubLog);
    
                resultsLog.println("Waiting for PUBCOMP reply...");
    
                pubCompEX = new PUBCOMPmsg(pubOut8.myId);
                pubComp = new PUBCOMPmsg(publishInput);
    
                SCADAutils.compareAndLog(pubComp, pubCompEX, pubLog, resultsLog);
    
                // We should NOT get back a publish msg in response
                try
                {
                    pubIn8 = new PUBLISHmsg(subscribeInput);
                    throw new ScadaException("Received a PUBLISH message when one should not have been available");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("No PUBLISH message available - good");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msg (Qos2)
            /*
             * We need to check QoS2 msg flow behaves as described in the SCADA
             * spec, if we do NOT send a PUBREC when getting a QoS2 publish msg -
             * the message should be resent
             */
            try
            {
                // Create publish data and publish
                pub9Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2) - no PUBREC sent on get");
                resultsLog.println("Creating publication message - QoS 2");
    
                pubOut9 = new PUBLISHmsg(topic, pub9Id, payload9, pub9Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut9.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn9EX = new PUBLISHmsg(topic, pub9Id, payload9, pub9Qos);
                pubIn9 = new PUBLISHmsg(subscribeInput);
                pubIn9EX.setId(pubIn9.myId);
    
                SCADAutils.compareAndLog(pubIn9, pubIn9EX, pubLog, resultsLog);
                resultsLog.println("Not sending PUBREC - should get resent msg...");

                // Drop the connection, wait then reconnect 
                // The QoS2 message should be resent
                subscribeSocket.close();
                Thread.sleep(5000);

                subscribeSocket = new Socket(hostName, port);

                resultsLog.println("Subscribe client reconnected");

                subscribeSocket.setSoTimeout(SCADAutils.timeout);

                resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

                subscribeOutput = new DataOutputStream(subscribeSocket.getOutputStream());
                subscribeInput = new DataInputStream(subscribeSocket.getInputStream());

                // Connect to broker and wait for reply

                resultsLog.printTitle("RECONNECTING TO BROKER");
                resultsLog.println("Creating CONNECT message");

                subscribeConnectMessage = new CONNECTmsg(subscribeClientId, false);
                resultsLog.println("Message created in " + connectLog.name);
                resultsLog.println("Sending CONNECT mesasge to broker for subscribe");
                subscribeConnectMessage.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);

                resultsLog.println("Waiting for published data to be resent - will acknowledge this one");
                pubIn9EX.setDUP();
                pubIn9 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn9EX);
                SCADAutils.compareAndLog(pubIn9, pubIn9EX, pubLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */
            // REMAKE SUBSCRIPTIONS
            /** *********************************************************** */
            // Create subscription list and subscribe
            resultsLog.printTitle("REMAKE SUBSCRIPTIONS");
            resultsLog.println("Creating subscription list");

            subList = new SubList(topic, subQos);

            subId = msgIds.getId();

            resultsLog.println("Subscriptions prepared");
            resultsLog.println("Creating subscripiton message");

            sub = new SUBSCRIBEmsg(subList, subId);

            resultsLog.println("SUBSCRIBE message created in " + subLog.name);
            resultsLog.println("Sending SUBSCRIBE message to broker...");

            sub.sendAndAcknowledge(subscribeInput, subscribeOutput, subLog, resultsLog);                

            /** *********************************************************** */
            // PUBLISH msg (Qos2)
            /*
             * We need to check QoS2 msg flow behaves as described in the SCADA
             * spec, if we do NOT send a PUBCOMP when getting a QoS2 publish msg -
             * the PUBREL message should be resent
             */
            try
            {
                // Create publish data and publish
                pub10Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2) - no PUBCOMP sent on get");
                resultsLog.println("Creating publication message - QoS 2");
    
                pubOut10 = new PUBLISHmsg(topic, pub10Id, payload10, pub10Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut10.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn10EX = new PUBLISHmsg(topic, pub10Id, payload10, pub10Qos);
                pubIn10 = new PUBLISHmsg(subscribeInput);
                pubIn10EX.setId(pubIn10.myId);
    
                SCADAutils.compareAndLog(pubIn10, pubIn10EX, pubLog, resultsLog);
                resultsLog.println("Sending PUBREC...");
    
                pubRec = new PUBRECmsg(pubIn10.myId);
    
                pubLog.println("PUBREC msg sent to broker (Qos2):");
                pubRec.parseAndPrint(pubLog);
    
                pubRec.send(subscribeOutput, pubLog);
    
                resultsLog.println("Waiting for PUBREL reply...");
    
                pubRelEX = new PUBRELmsg(pubIn10.myId);
                pubRel = new PUBRELmsg(subscribeInput);
    
                SCADAutils.compareAndLog(pubRel, pubRelEX, pubLog, resultsLog, true);
                resultsLog.println("Not sending PUBCOMP - should get a new PUBREL msg...");

                // Drop the connection, wait then reconnect 
                // The QoS2 message should be resent
                subscribeSocket.close();
                Thread.sleep(5000);

                subscribeSocket = new Socket(hostName, port);

                resultsLog.println("Subscribe client reconnected");

                subscribeSocket.setSoTimeout(SCADAutils.timeout);

                resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

                subscribeOutput = new DataOutputStream(subscribeSocket.getOutputStream());
                subscribeInput = new DataInputStream(subscribeSocket.getInputStream());

                // Connect to broker and wait for reply

                resultsLog.printTitle("RECONNECTING TO BROKER");
                resultsLog.println("Creating CONNECT message");

                subscribeConnectMessage = new CONNECTmsg(subscribeClientId, false);
                resultsLog.println("Message created in " + connectLog.name);
                resultsLog.println("Sending CONNECT mesasge to broker for subscribe");
                subscribeConnectMessage.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);

                resultsLog.println("Waiting for published data to be resent - will acknowledge this one");
    
                pubRel = new PUBRELmsg(subscribeInput);
                pubRelEX.setDUP(true);
                SCADAutils.compareAndLog(pubRel, pubRelEX, pubLog, resultsLog);
                resultsLog.println("PUBREL received - sending final PUBCOMP...");
    
                pubComp = new PUBCOMPmsg(pubIn10.myId);
    
                pubLog.println("PUBCOMP msg sent to broker (Qos2):");
                pubComp.parseAndPrint(pubLog);
    
                pubComp.send(subscribeOutput, pubLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // DISCONNECT msgs
            resultsLog.printTitle("TESTING DISCONNECT MESSAGES");
            resultsLog.println("Preparing DISCONNECT msg");

            disMsg = new DISCONNECTmsg();

            resultsLog.println("DISCONNECT message created in " + connectLog.name);
            resultsLog.println("Sending DISCONNECT message to broker...");

            disMsg.send(subscribeOutput, connectLog);
            disMsg.send(publishOutput, connectLog);

            resultsLog.printTitle("END OF TEST");
        }
        catch (Exception e)
        {
            testPassed = false;
            e.printStackTrace(resultsLog.out);
        }
        finally
        {
            resultsLog.println(testPassed ? "TEST PASSED" : "TEST FAILED");
            System.exit(testPassed ? 0 : 1);
        }

        /** *********************************************************** */
        // END TEST
        /** *********************************************************** */

    }
}
