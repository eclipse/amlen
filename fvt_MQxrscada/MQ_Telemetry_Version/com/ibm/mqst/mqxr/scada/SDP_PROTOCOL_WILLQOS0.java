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

public class SDP_PROTOCOL_WILLQOS0
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_WILLQOS0";
        String hostName = "localhost";
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
        Socket client1;
        DataOutputStream output1;
        DataInputStream input1;
        Socket client2;
        DataOutputStream output2;
        DataInputStream input2;

        // Messages // Expected messages
        CONNECTmsg connect1;
        CONNECTmsg connect2;
        SUBSCRIBEmsg sub;

        PUBLISHmsg pubOut2; // Qos 1 - RETAIN SET

        PUBLISHmsg pubIn1;
        PUBLISHmsg pubIn1EX; // Qos 0 - get (RETAINED) on subscribe

        DISCONNECTmsg disMsg;

        // Support classes
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;

        // Test data
        String clientId1 = "C1WillQoS0";
        String clientId2 = "C2WillQoS0";
         
        String willTopic = "WillQos0";
        String willMessage = "THIS IS THE WILL MESSAGE";
        byte willQos = 0;

        try
        {

            /** *********************************************************** */
            // SETUP AND CONNECT TO CLIENTS
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

            resultsLog.println("Creating sockets...");

            client1 = new Socket(hostName, port);

            resultsLog.println("Connected");

            client1.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            output1 = new DataOutputStream(client1.getOutputStream());
            input1 = new DataInputStream(client1.getInputStream());

            client2 = new Socket(hostName, port);

            resultsLog.println("Connected");

            client2.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            output2 = new DataOutputStream(client2.getOutputStream());
            input2 = new DataInputStream(client2.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER AS CLIENT1 - WILL INFORMATION SUPPLIED");
            resultsLog.println("Creating CONNECT message");

            connect1 = new CONNECTmsg(clientId1, willTopic, willMessage, willQos);

            connect1.setKeepAlive((short) 10);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT mesasge to broker");

            connect1.sendAndAcknowledge(input1, output1, connectLog, resultsLog);

            resultsLog.printTitle("CONNECTING TO BROKER AS CLIENT2");
            resultsLog.println("Creating CONNECT message");

            connect2 = new CONNECTmsg(clientId2);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT mesasge to broker");

            connect2.sendAndAcknowledge(input2, output2, connectLog, resultsLog);

            /** *********************************************************** */
            // SUBSCRIBE 2ND CLIENT TO WILL TOPIC
            /** *********************************************************** */

            // Create subscription list and subscribe
            resultsLog.printTitle("SUBSCRIPTIONS FOR WILL TOPIC");
            resultsLog.println("Creating subscription list");

            subList = new SubList(willTopic, willQos);

            subId = msgIds.getId();

            resultsLog.println("Subscriptions prepared");
            resultsLog.println("Creating subscripiton message");

            sub = new SUBSCRIBEmsg(subList, subId);

            resultsLog.println("SUBSCRIBE message created in " + subLog.name);
            resultsLog.println("Sending SUBSCRIBE message to broker...");

            sub.sendAndAcknowledge(input2, output2, subLog, resultsLog);

            /** *********************************************************** */
            // DISCONNECT FIRST CLIENT
            /** *********************************************************** */

            // Close Socket
            resultsLog.printTitle("CLOSING FIRST CLIENT");

            client1.close();

            /** *********************************************************** */
            // GET WILL INFORMATION
            /** *********************************************************** */

            // PUBLISH msg (Qos0)
            resultsLog.printTitle("GETTING WILL PUBLISH MESSAGES");
            resultsLog.println("Waiting for QoS0 message...");

            pubIn1EX = new PUBLISHmsg(willTopic, willMessage.getBytes());
            pubIn1 = new PUBLISHmsg(input2);

            SCADAutils.compareAndLog(pubIn1, pubIn1EX, pubLog, resultsLog);

            /** *********************************************************** */
            // DISCONNECT
            /** *********************************************************** */

            // DISCONNECT msgs
            resultsLog.printTitle("TESTING DISCONNECT MESSAGES");
            resultsLog.println("Preparing DISCONNECT msg");

            disMsg = new DISCONNECTmsg();

            resultsLog.println("DISCONNECT message created in " + connectLog.name);
            resultsLog.println("Sending DISCONNECT message to broker...");

            disMsg.send(output2, connectLog);

            resultsLog.printTitle("END OF TEST");
            resultsLog.println("TEST PASSED!");
            System.exit(0);

        }
        catch (IOException e)
        {
            e.printStackTrace(resultsLog.out);
            resultsLog.println("TEST FAILED");
            System.exit(1);

        }
        catch (ScadaException e)
        {
            e.printStackTrace(resultsLog.out);
            resultsLog.println("TEST FAILED");
            System.exit(1);
        }

        /** *********************************************************** */
        // END TEST
        /** *********************************************************** */

    }
}
