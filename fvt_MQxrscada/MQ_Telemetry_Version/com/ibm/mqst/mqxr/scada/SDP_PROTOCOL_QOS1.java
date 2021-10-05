package com.ibm.mqst.mqxr.scada;
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


import java.io.*;
import java.net.*;

public class SDP_PROTOCOL_QOS1
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_QOS1";
        String hostName = "localhost";
        boolean testPassed = true;
        
        int port = 1883;
        String arg;
        int i = 0;
     
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

        // Messages
        CONNECTmsg publishConnectMessage;
        CONNECTmsg subscribeConnectMessage;
        SUBSCRIBEmsg sub;

        PUBLISHmsg pubOut1; // Qos 0
        PUBLISHmsg pubOut2; // Qos 1 - normal
        PUBLISHmsg pubOut3; // Qos 1 - ack ignored
        PUBLISHmsg pubOut5; // Qos 1 - reserved id
        PUBLISHmsg pubOut6; // Qos 2

        PUBLISHmsg pubIn1;
        PUBLISHmsg pubIn1EX; // Qos 0
        PUBLISHmsg pubIn2;
        PUBLISHmsg pubIn2EX; // Qos 1 - normal
        PUBLISHmsg pubIn3;
        PUBLISHmsg pubIn3EX; // Qos 1 - no ack
        PUBLISHmsg pubIn4;
        PUBLISHmsg pubIn4EX; // Qos 1 - finally acked
        PUBLISHmsg pubIn5;
        PUBLISHmsg pubIn6;
        PUBLISHmsg pubIn6EX; // Qos 2

        DISCONNECTmsg disMsg;

        // Support classes
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;
        MsgIdentifier pub2Id; // Qos 1 - normal
        MsgIdentifier pub3Id; // Qos 1 - ack ignored
        MsgIdentifier pub5Id; // Qos 1 - reserved id
        MsgIdentifier pub6Id; // Qos 2

        // Test data
        String publishClientId = "C1QoS1Pub";
        String subscribeClientId = "C2QoS1Sub";
         
        String topic = "SamQos1";
        byte[] payload1 = "This is payload 1".getBytes();
        byte[] payload2 = "This is payload 2".getBytes();
        byte[] payload3 = "This is payload 3".getBytes();
        byte pub2Qos = 1;
        byte pub3Qos = 1;
        byte pub5Qos = 1;
        byte pub6Qos = 2;

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
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("SUBSCRIPTIONS");
                resultsLog.println("Creating subscription list");
    
                subList = new SubList(topic, pub2Qos);
    
                subId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscripiton message");
    
                sub = new SUBSCRIBEmsg(subList, subId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscribeInput, subscribeOutput, subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

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
             * We need to check that a QoS1 publish msgs a) Work as they should
             * given proper acknowledgements b) Work as they should if PUBACKs
             * are not sent (incoming publish msgs only)
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
             * We need to check that Qos1 publish msg flow behaves as described
             * in the spec - if the ACK is not sent on receipt of a Qos1 msg,
             * then the broker should resend it
             */
              try
                {
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
           
                resultsLog.println("Not sending PUBACK - drop client connection by closing socket");

                // Drop the connection, wait then reconnect 
                // The QoS1 message should be delivered
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
            // RE-SUBSCRIBE
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("REMAKE SUBSCRIPTIONS");
                resultsLog.println("Creating subscription list");
    
                subList = new SubList(topic, pub2Qos);
    
                subId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscripiton message");
    
                sub = new SUBSCRIBEmsg(subList, subId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscribeInput, subscribeOutput, subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msg (Qos1)
            /*
             * We need to check that a QoS1 publish msg using the reserved id -
             * 0 is treated as illegal, and NOT returned to subscribers
             */
/*
            try
            {
                // Create publish data and publish - normal acknowledgments
    
                pub5Id = msgIds.getId(0);
                
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos1) - reserved ID"); 
                resultsLog.println("Creating publication message - QoS 1 - reserved ID");
                
                pubOut5 = new PUBLISHmsg(topic, pub5Id, payload2, pub5Qos);
                
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
                
                pubOut5.send(publishOutput, pubLog);
                pubOut5.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should NOT get a PUBLISH msg back
                try
                {
                    pubIn5 = new PUBLISHmsg(subscribeInput);
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
             * We need to check that a QoS2 publish msg is 'downgraded' to QoS1
             * if that is what the client is subscribed at
             */
            try
            {
                // Create publish data and publish
                pub6Id = msgIds.getId();
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2)");
                resultsLog.println("Creating publication message - QoS 2");
    
                pubOut6 = new PUBLISHmsg(topic, pub6Id, payload3, pub6Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut6.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn6EX = new PUBLISHmsg(topic, pub6Id, payload3, pub5Qos);
                pubIn6 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn6EX);
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
