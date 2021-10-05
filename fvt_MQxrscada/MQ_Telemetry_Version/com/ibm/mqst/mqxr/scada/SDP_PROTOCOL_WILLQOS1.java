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

public class SDP_PROTOCOL_WILLQOS1
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_WILLQOS1";
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

        PUBLISHmsg pubIn1;
        PUBLISHmsg pubIn1EX; // WILL information from Client1 at QoS 1

        DISCONNECTmsg disMsg;

        // Support classes
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;
        MsgIdentifier pub1Id; // Qos 1

        // Test data
        String clientId1 = "C1WillQoS1";
        String clientId2 = "C2WillQoS1";
         
        String willTopic = "WillQos1";
        String willMessage = "THIS IS THE WILL MESSAGE";
        // byte[] payload1 = SCADAutils.StringToUTF(willMessage);
        byte[] payload1 = willMessage.getBytes();
        byte willQos = 1;

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

            // PUBLISH msg (Qos1)
            resultsLog.printTitle("GETTING WILL PUBLISH MESSAGES");
            resultsLog.println("Waiting for QoS1 message...");

            pub1Id = msgIds.getId();
            pubIn1EX = new PUBLISHmsg(willTopic, pub1Id, payload1, willQos);
            pubIn1 = new PUBLISHmsg(input2, output2, pubLog, resultsLog, pubIn1EX);

            /** *********************************************************** */
            // DISCONNECT SECOND CLIENT
            /** *********************************************************** */

            // DISCONNECT msgs
            resultsLog.printTitle("TESTING DISCONNECT MESSAGES");
            resultsLog.println("Preparing DISCONNECT msg");

            disMsg = new DISCONNECTmsg();

            // prepare test message before disconnect to minimise time between
            // disconnect and publish.
            MsgIdentifier tmp = msgIds.getId();
            PUBLISHmsg pubOut2 = new PUBLISHmsg("JUNK", tmp, "JUNK PAYLOAD".getBytes(), (short) 1);

            resultsLog.println("DISCONNECT message created in " + connectLog.name);
            resultsLog.println("Sending DISCONNECT message to broker...");

            disMsg.send(output2, connectLog);

            // Most of the SCADA node tests originally attempted to verify that 
            // the disconnect message had been actioned by publishing a QoS1 
            // message to the socket immediately after the disconnect. The tests 
            // expected to get either a Socket or EoF exception to indicate that 
            // the Broker had closed the socket. MQXR appears to be less 
            // aggressive than Message Broker in closing the socket so,  
            // in some cases, the PUBLISH reaches MQXR and is registered as a 
            // protocol error. One or more FDCs are generated and the client id is 
            // left in an error state, which should be cleared when the same client 
            // reconnects (originally further FDCs were generated on reconnect).
            // The combination of this effect with the use of the same client ids 
            // in different tests resulted in multiple FDCs. In order to make the 
            // test cases independent of one another, I have removed the publish 
            // used for disconnect verification from all test cases except this one.
            // I have also made the client ids unique to each test case.
            // In this test case, I have left the publish after the disconnect for
            // client 2 and added an extra connect/disconnect sequence to confirm 
            // that the error state is cleared. The successful clearing of the 
            // the error state is inferred from the number of FDCs registered
            // by ST001 after this test case has run.

            try
            {               
                pubOut2.sendAndAcknowledge(input2, output2, pubLog, resultsLog);
                throw new ScadaException("Didn't get expected exception");
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
            
            /** *********************************************************** */
            // Now reconnect and disconnect both clients again. MQXR should
            // clear any the state error for Client 2 and carry on with  
            // no further FDCs.
            /** *********************************************************** */
            
            SCADAutils.sleep(3000);
            
            resultsLog.printTitle("RECONNECTING CLIENTS TO BROKER TO CLEAR ERROR STATE");            
            
            client1 = new Socket(hostName, port);            
            client2 = new Socket(hostName, port);

            resultsLog.println("Reconnected");

            output1 = new DataOutputStream(client1.getOutputStream());
            input1 = new DataInputStream(client1.getInputStream());            
            
            output2 = new DataOutputStream(client2.getOutputStream());
            input2 = new DataInputStream(client2.getInputStream());
   
            resultsLog.println("Creating CONNECT messages");

            connect1 = new CONNECTmsg(clientId1);            
            connect2 = new CONNECTmsg(clientId2);

            resultsLog.println("Messages created in " + connectLog.name);
            resultsLog.println("Sending CONNECT mesasges to broker");

            connect1.sendAndAcknowledge(input1, output1, connectLog, resultsLog);            
            connect2.sendAndAcknowledge(input2, output2, connectLog, resultsLog);
            
            resultsLog.println("Sending DISCONNECT messages to broker...");

            disMsg.send(output1, connectLog);            
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
