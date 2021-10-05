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

public class SDP_PROTOCOL_PACKED
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_PACKED";
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

        // Messages // Expected messages
        CONNECTmsg publishConnectMessage;
        CONNECTmsg subscribeConnectMessage;
        SUBSCRIBEmsg sub;
        PUBLISHmsg pubOut1;
        PUBLISHmsg pubIn1;
        PUBLISHmsg pubIn1EX;
        DISCONNECTmsg disMsg;

        // Support classes
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;

        // Test data
        String publishClientId = "noretain@C1PackedPub";
        String subscribeClientId = "C2PackedSub";
       
        String topic1 = "SamQos0";
        byte[] payload1 = "This is payload 1".getBytes();
        byte pub1Qos = 0;
 
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
            
            // Create socket and setup data streams
            resultsLog.printTitle("Starting test " + testName + " Version="+SCADAutils.version);
            System.out.println("Starting test " + testName + " Version="+SCADAutils.version);
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);

            subscribeSocket = new Socket(hostName, port);
            publishSocket = new Socket(hostName, port);

            resultsLog.println("Connected");

            subscribeSocket.setSoTimeout(30000000);
            publishSocket.setSoTimeout(30000000/*SCADAutils.timeout*/);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            subscribeOutput = new DataOutputStream(subscribeSocket.getOutputStream());
            subscribeInput = new DataInputStream(subscribeSocket.getInputStream());
            //publishOutput = new DataOutputStream(publishSocket.getOutputStream());
            //publishInput = new DataInputStream(publishSocket.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER");
            resultsLog.println("Creating CONNECT message");

            subscribeConnectMessage = new CONNECTmsg(subscribeClientId);
            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker for subscribe");
            subscribeConnectMessage.sendAndAcknowledge(subscribeInput, subscribeOutput, connectLog, resultsLog);

            //publishConnectMessage = new CONNECTmsg(publishClientId);
            //resultsLog.println("Message created in " + connectLog.name);
            //resultsLog.println("Sending CONNECT message to broker for publish");
            //publishConnectMessage.sendAndAcknowledge(publishInput, publishOutput, connectLog, resultsLog);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker");


            /** *********************************************************** */
            // SUBSCRIBE
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("SUBSCRIPTIONS");
                resultsLog.println("Creating subscription list");
    
                subList = new SubList(topic1, pub1Qos);
    
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
             * We need to test that a msg published at QoS0 is "at most once" -
             * not a lot to do bar sending one off and expecting a reply. Then
             * we continue the test
             */
            try
            {
            	// Create ByteArrayOutputStream to pack messages into
            	ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
            	publishOutput = new DataOutputStream(byteStream);
            	
            	// Create and write the CONNECT message
            	publishConnectMessage = new CONNECTmsg(publishClientId);
                resultsLog.println("Message created in " + connectLog.name);
                resultsLog.println("Sending CONNECT message to broker for publish");
                publishConnectMessage.send(publishOutput, connectLog);
                
                // Create and write a PUBLISH message
            	
                // Create publish data and publish
                resultsLog.printTitle("TESTING PUBLISH MESSAGES (Qos0)");
                resultsLog.println("Creating publication message - QoS 0");
    
                pubOut1 = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut1.send(publishOutput, pubLog);
                
                // And create and write a DISCONNECT message

                disMsg = new DISCONNECTmsg();
                disMsg.send(publishOutput, connectLog);

                // New we need to send it all down the line
                publishSocket.getOutputStream().write(byteStream.toByteArray());
    
                // Should now get the PUBLISH msg back - we are subscribed
    
                resultsLog.println("Waiting for published data");
    
                pubIn1EX = new PUBLISHmsg(topic1, payload1);
                pubIn1 = new PUBLISHmsg(subscribeInput);
    
                SCADAutils.compareAndLog(pubIn1, pubIn1EX, pubLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


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
