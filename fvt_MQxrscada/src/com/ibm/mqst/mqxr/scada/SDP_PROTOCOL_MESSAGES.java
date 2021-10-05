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
import java.net.SocketTimeoutException;

public class SDP_PROTOCOL_MESSAGES
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_MESSAGES";
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
        PINGREQmsg ping;
        SUBSCRIBEmsg sub;
        UNSUBSCRIBEmsg unSub;
        PUBLISHmsg pub1;
        PUBLISHmsg pub2;
        PUBLISHmsg pub3;
        PUBLISHmsg pubIn1;
        PUBLISHmsg pubIn2;
        PUBLISHmsg pubIn3;
        PUBLISHmsg pubIn4;
        PUBLISHmsg pubIn1EX;
        PUBLISHmsg pubIn2EX;
        PUBLISHmsg pubIn3EX;
        DISCONNECTmsg disMsg;

        // Support classes
        SubList subList;
        TopicList topList;
        MsgIdentifier pub2Id;
        MsgIdentifier pub3Id;
        MsgIdentifier unsubId;

        // Test data
        String publishClientId = "C1MessagesPub";
        String subscribeClientId = "C2MessagesSub";
    
        String topic1 = "MessagesQos0";
        String topic2 = "MessagesQos1";
        String topic3 = "MessagesQos2";
        byte[] payload1 = "This is payload 1".getBytes();
        byte[] payload2 = "This is payload 2".getBytes();
        byte[] payload3 = "This is payload 3".getBytes();
        byte pub1Qos = 0;
        byte pub2Qos = 1;
        byte pub3Qos = 2;

        try
        {

            /** *********************************************************** */
            // SETUP
            /** *********************************************************** */

            resultsLog = new LogFile(resultsLogName);
            connectLog = new LogFile(connectLogName);
            subLog = new LogFile(subLogName);
            pubLog = new LogFile(pubLogName);

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
                    resultsLog = new LogFile("stdout");
                }
            }   
            
            resultsLog.printTitle("Starting test " + testName + " Version="+SCADAutils.version);
            System.out.println("Starting test " + testName + " Version="+SCADAutils.version);
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

            /** *********************************************************** */
            // TEST MESSAGE TYPES
            /** *********************************************************** */

            // CONNECT msgs
            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER");
            resultsLog.println("Creating CONNECT message");

            subscribeConnectMessage = new CONNECTmsg(subscribeClientId);
            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker for subscribe");
            subscribeConnectMessage.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);

            publishConnectMessage = new CONNECTmsg(publishClientId);
            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker for publish");
            publishConnectMessage.sendAndAcknowledge(publishInput, publishOutput, connectLog, resultsLog);

            /** *********************************************************** */

            // PING msgs
            try
            {
                resultsLog.printTitle("TESTING PING MESSAGES");
                resultsLog.println("Try to ping broker - creating PINGREQ message");
    
                ping = new PINGREQmsg();
    
                resultsLog.println("Sending PINGREQ message to broker...");
    
                ping.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // SUBSCRIBE msgs
            // Create subscription list and subscribe
            try
            {
                resultsLog.printTitle("TESTING SUBSCRIBE MESSAGES");
                resultsLog.println("Creating subscription list");
    
                subList = new SubList(topic1, pub1Qos);
                subList.addSub(topic2, pub2Qos);
                subList.addSub(topic3, pub3Qos);
    
                MsgIdentifier subID = new MsgIdentifier(1);
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscripiton message");
    
                sub = new SUBSCRIBEmsg(subList, subID);
    
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

            // PUBLISH msgs (Qos0)
            // Create publish data and publish
            try
            {
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos0)");
                resultsLog.println("Creating publication message - QoS 0");
    
                pub1 = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pub1.send(publishOutput, pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn1EX = new PUBLISHmsg(topic1, payload1);
                pubIn1 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn1EX);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

            // PUBLISH msgs (Qos1)
            // Create publish data and publish
            try
            {
                pub2Id = new MsgIdentifier(2);
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos1)");
                resultsLog.println("Creating publication message - QoS 1");
    
                pub2 = new PUBLISHmsg(topic2, pub2Id, payload2, pub2Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pub2.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn2EX = new PUBLISHmsg(topic2, pub2Id, payload2, pub2Qos);
                pubIn2 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn2EX);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
    

            /** *********************************************************** */

            // PUBLISH msgs (Qos2)
            // Create publish data and publish
            try
            {
                pub3Id = new MsgIdentifier(9);
    
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos2)");
                resultsLog.println("Creating publication message - QoS 2");
    
                pub3 = new PUBLISHmsg(topic3, pub3Id, payload3, pub3Qos);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pub3.sendAndAcknowledge(publishInput, publishOutput, pubLog, resultsLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn3EX = new PUBLISHmsg(topic3, pub3Id, payload3, pub3Qos);
                pubIn3 = new PUBLISHmsg(subscribeInput, subscribeOutput, pubLog, resultsLog, pubIn3EX);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // UNSUBSCRIBE msgs
            try
            {
                unsubId = new MsgIdentifier(4);
                topList = new TopicList(topic1);
    
                resultsLog.printTitle("TESTING UNSUBSCRIBE MESSAGES");
                resultsLog.println("Preparing UNSUBSCRIBE msg");
    
                unSub = new UNSUBSCRIBEmsg(topList, unsubId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unSub.sendAndAcknowledge(subscribeInput, subscribeOutput, subLog, resultsLog);
    
                // Now we need to test unsubscribe worked - try Qos0 publish to
                // topic1
    
                resultsLog.println("Trying PUBLISH to unsubscribed topic - should be no response");
    
                pub1 = new PUBLISHmsg(topic1, payload1);
                pub1.send(publishOutput, pubLog);
                
                // Should NOT get a PUBLISH msg back
                try
                {
                    pubIn4 = new PUBLISHmsg(subscribeInput);
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

            // DISCONNECT msgs
            resultsLog.printTitle("TESTING DISCONNECT MESSAGES");
            resultsLog.println("Preparing DISCONNECT msg");

            disMsg = new DISCONNECTmsg();

            resultsLog.println("DISCONNECT message created in " + connectLog.name);
            resultsLog.println("Sending DISCONNECT message to broker...");
            
            disMsg.send(subscribeOutput, connectLog);            


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
            System.out.println(testName + (testPassed ? " PASSED" : " FAILED"));
            if (!testmode && testPassed) {
                connectLog.delete();
                subLog.delete();
                pubLog.delete();
            }
            System.exit(testPassed ? 0 : 1);
        }
		        

        /** *********************************************************** */
        // END TEST
        /** *********************************************************** */

    }
}
