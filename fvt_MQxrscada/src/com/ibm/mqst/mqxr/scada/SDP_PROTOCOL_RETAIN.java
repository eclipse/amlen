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

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;

public class SDP_PROTOCOL_RETAIN
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_RETAIN";
        String hostName = "localhost";
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
        Socket client1;
        DataOutputStream output1;
        DataInputStream input1;
        Socket client2;
        DataOutputStream output2;
        DataInputStream input2;

        // Messages // Expected messages
        CONNECTmsg msg;
        SUBSCRIBEmsg sub;

        PUBLISHmsg pubOut1; // Qos 0 - RETAIN SET
        PUBLISHmsg pubOut2; // Qos 1 - RETAIN SET
        PUBLISHmsg pubOut3; // Qos 2 - RETAIN SET

        PUBLISHmsg pubIn1EX; // Qos 0 - get (RETAINED) on subscribe
        PUBLISHmsg pubIn2EX; // Qos 1 - get (RETAINED) on subscribe
        PUBLISHmsg pubIn3EX; // Qos 2 - get (RETAINED) on subscribe

        DISCONNECTmsg disMsg;

        // Support classes
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;
        MsgIdentifier pub2Id; // Qos 1
        MsgIdentifier pub3Id; // Qos 2

        // Test data
        String clientId1 = "C1Retain";
        String clientId2 = "C2Retain";
     
        String topic1 = "SamQos0Retain";
        String topic2 = "SamQos1Retain";
        String topic3 = "SamQos2Retain";
        byte[] payload1 = "QoS0 RETAINED Message".getBytes();
        byte[] payload2 = "QoS1 RETAINED Message".getBytes();
        byte[] payload3 = "QoS2 RETAINED Message".getBytes();
        byte pub1Qos = 0;
        byte pub2Qos = 1;
        byte pub3Qos = 2;

        try
        {

            /** *********************************************************** */
            // SETUP AND CONNECT 1st CLIENT
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
            resultsLog.println("Connecting Client1");
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);

            client1 = new Socket(hostName, port);

            resultsLog.println("Connected");

            client1.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            output1 = new DataOutputStream(client1.getOutputStream());
            input1 = new DataInputStream(client1.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER");
            resultsLog.println("Creating CONNECT message");

            msg = new CONNECTmsg(clientId1);

            msg.setKeepAlive((short) 1);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker");

            msg.sendAndAcknowledge(input1, output1, connectLog, resultsLog);

            /** *********************************************************** */
            // PUBLISH MSGS WITH RETAIN SET
            /** *********************************************************** */

            // PUBLISH msg (Qos0)
            // Create publish data and publish
            resultsLog.printTitle("TESTING RETAINED PUBLISH MESSAGES");
            resultsLog.println("Creating publication message - QoS 0");

            pubOut1 = new PUBLISHmsg(topic1, payload1);
            pubOut1.setRetain();

            resultsLog.println("PUBLISH message created in " + pubLog.name);
            resultsLog.println("Sending PUBLISH message to broker...");

            pubOut1.send(output1, pubLog);

            // PUBLISH msg (Qos1)

            pub2Id = msgIds.getId();

            // Create publish data and publish

            resultsLog.println("Creating publication message - QoS 1");

            pubOut2 = new PUBLISHmsg(topic2, pub2Id, payload2, pub2Qos);
            pubOut2.setRetain();
            pubOut2.setNoSubscription(true);

            resultsLog.println("PUBLISH message created in " + pubLog.name);
            resultsLog.println("Sending PUBLISH message to broker...");

            pubOut2.sendAndAcknowledge(input1, output1, pubLog, resultsLog);

            // PUBLISH msg (Qos2)

            pub3Id = msgIds.getId();

            // Create publish data and publish

            resultsLog.println("Creating publication message - QoS 2");

            pubOut3 = new PUBLISHmsg(topic3, pub3Id, payload3, pub3Qos);
            pubOut3.setRetain();
            pubOut3.setNoSubscription(true);

            resultsLog.println("PUBLISH message created in " + pubLog.name);
            resultsLog.println("Sending PUBLISH message to broker...");

            pubOut3.sendAndAcknowledge(input1, output1, pubLog, resultsLog);

            /** *********************************************************** */
            // DISCONNECT
            /** *********************************************************** */

            // DISCONNECT msgs
            resultsLog.printTitle("TESTING DISCONNECT MESSAGES");
            resultsLog.println("Preparing DISCONNECT msg");

            disMsg = new DISCONNECTmsg();

            resultsLog.println("DISCONNECT message created in " + connectLog.name);
            resultsLog.println("Sending DISCONNECT message to broker...");

            disMsg.send(output1, connectLog);

            //TODO Now we need to test the DISCONNECT was registered

            //resultsLog.println("Message sent - trying a Qos1 PUBLISH...");

            // We should get an Exception when reading from the socket

            //try
            //{

            //    pubOut2.sendAndAcknowledge(input1, output1, pubLog, resultsLog);
            //    throw new ScadaException("Didn't receive expected exception");
            //}
            // either SocketException or EOFException is acceptable here
            //catch (SocketException e)
            //{
            //    resultsLog.println("Expected SocketException received - DISCONNECT was recognised");
            //}
            //catch (EOFException e)
            //{
            //    resultsLog.println("Expected EOFException received - DISCONNECT was recognised");
            //}

            SCADAutils.sleep(1000);

            /** *********************************************************** */
            // CONNECT 2nd CLIENT
            /** *********************************************************** */

            // Create socket and setup data streams
            resultsLog.println("Connecting Client2");
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);

            client2 = new Socket(hostName, port);

            resultsLog.println("Connected");

            client2.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            output2 = new DataOutputStream(client2.getOutputStream());
            input2 = new DataInputStream(client2.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER");
            resultsLog.println("Creating CONNECT message");

            msg = new CONNECTmsg(clientId2);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker");

            msg.sendAndAcknowledge(input2, output2, connectLog, resultsLog);

            /** *********************************************************** */
            // SUBSCRIBE
            /** *********************************************************** */

            // Create subscription list and subscribe
            resultsLog.printTitle("SUBSCRIBTIONS");
            resultsLog.println("Creating subscription list");

            subList = new SubList(topic1, pub1Qos);
            subList.addSub(topic2, pub2Qos);
            subList.addSub(topic3, pub3Qos);

            subId = msgIds.getId();

            resultsLog.println("Subscriptions prepared");
            resultsLog.println("Creating subscripiton message");

            sub = new SUBSCRIBEmsg(subList, subId);

            resultsLog.println("SUBSCRIBE message created in " + subLog.name);
            resultsLog.println("Sending SUBSCRIBE message to broker...");

            sub.send(output2, subLog);

            /** *********************************************************** */
            // GET RETAINED PUBLICATIONS
            /** *********************************************************** */

            // SUBACK and retained messages can arrive in any order- read in all
            // messages and check once all have arrived
            resultsLog.printTitle("GETTING RETAINED PUBLISH MESSAGES");
            
            ArrayList<MsgTemplate> receivedMsgs = new ArrayList<MsgTemplate>();
            receivedMsgs.add(SCADAutils.getGenericMsg(input2));
            receivedMsgs.add(SCADAutils.getGenericMsg(input2));
            receivedMsgs.add(SCADAutils.getGenericMsg(input2));
            receivedMsgs.add(SCADAutils.getGenericMsg(input2));

            // The msgId must be read out of the QoS 1 and 2 messages so they can be fed
            // into the expected messages
            // NB QoS is not available in generic messages and so have to key off the topic
            for (int j = 0; j < receivedMsgs.size(); j++) {
                MsgTemplate rMsg = receivedMsgs.get(j);
                if (rMsg instanceof PUBLISHmsg) {
                    PUBLISHmsg pMsg = (PUBLISHmsg) rMsg;
                    if (Arrays.equals(SCADAutils.StringToUTF(topic2), pMsg.topic)) {
                        pub2Id = new MsgIdentifier(pMsg.msgId);
                    } else if (Arrays.equals(SCADAutils.StringToUTF(topic3), pMsg.topic)) {
                        pub3Id = new MsgIdentifier(pMsg.msgId);
                    }
                }
            }
            
            // SUBACK msg
            SUBACKmsg subAckEX = new SUBACKmsg(subList, subId);

            // PUBLISH msg (Qos0)
            pubIn1EX = new PUBLISHmsg(topic1, payload1);
            pubIn1EX.setRetain();

            // PUBLISH msg (Qos1)
            pubIn2EX = new PUBLISHmsg(topic2, pub2Id, payload2, pub2Qos);
            pubIn2EX.setRetain();

            // PUBLISH msg (Qos2)
            pubIn3EX = new PUBLISHmsg(topic3, pub3Id, payload3, pub3Qos);
            pubIn3EX.setRetain();

            ArrayList<MsgTemplate> expectedMsgs = new ArrayList<MsgTemplate>();
            expectedMsgs.add(pubIn1EX);
            expectedMsgs.add(pubIn2EX);
            expectedMsgs.add(pubIn3EX);
            expectedMsgs.add(subAckEX);

            SCADAutils.compareAndLog(receivedMsgs, expectedMsgs, pubLog, resultsLog, input2, output2, true);
            
            resultsLog.printTitle("Unset retained messages");
            pubOut1 = new PUBLISHmsg(topic1, new byte[0]);
            pubOut1.setRetain();
            pubOut1.send(output2, pubLog);
            resultsLog.println("Unset retain 0");
            pubOut1 = new PUBLISHmsg(topic2, new byte[0]);
            pubOut1.setRetain();
            pubOut1.send(output2, pubLog);
            resultsLog.println("Unset retain 1");
            pubOut1 = new PUBLISHmsg(topic3, new byte[0]);
            pubOut1.setRetain();
            pubOut1.send(output2, pubLog);
            resultsLog.println("Unset retain 2");
            SCADAutils.sleep(1000);
            

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
            resultsLog.println("TEST PASSED");
            System.out.println(testName + " PASSED");
            if (!testmode) {
                connectLog.delete();
                subLog.delete();
                pubLog.delete();
            }
            System.exit(0);

        }
        catch (IOException e)
        {
            e.printStackTrace(resultsLog.out);
            resultsLog.println("TEST FAILED");
            System.out.println(testName + " FAILED");
            System.exit(1);
        }
        catch (ScadaException e)
        {
            e.printStackTrace(resultsLog.out);
            resultsLog.println("TEST FAILED");
            System.out.println(testName + " FAILED");
            System.exit(1);
        }

        /** *********************************************************** */
        // END TEST
        /** *********************************************************** */

    }
}
