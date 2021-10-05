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

public class SDP_PROTOCOL_NETWORK
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_NETWORK";
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

        // Support classes
        Client subscriber;
        Client publisher;
        MsgIdHandler msgIds;
        SubList subList;
        MsgIdentifier subId;

        // Test data
        String subscriberClientId = "C1NetworkSub";
        String publisherClientId = "C2NetworkPub";
               
        String topic1 = "NetworkTopic";
        byte[] payload1 = "This is payload 1".getBytes();
        byte qos0 = 0;

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
            
            resultsLog.printTitle("Starting test " + testName + " version="+SCADAutils.version);
            System.out.println("Starting test " + testName + " version="+SCADAutils.version);
            
            // Create socket and setup data streams
            resultsLog.println("Connecting to host: " + hostName + " at port: " + port);
            publisher = new Client(hostName, port);
            subscriber = new Client(hostName, port);
            resultsLog.println("Connected");

            /** *********************************************************** */
            // Ignore CONNACK and resend CONNECT- should get disconnected
            /** *********************************************************** */
            try
            {
                resultsLog.printTitle("TESTING IGNORE CONNACK AND RESEND CONNECT");
                resultsLog.println("Connecting publisher");
                msg = new CONNECTmsg(publisherClientId);
                msg.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), connectLog, resultsLog);
    
                resultsLog.println("Ignoring CONNACK and sleeping for 5 seconds");
                SCADAutils.sleep(5000);
                resultsLog.println("Resending CONNECT message");
                try
                {
                    if (SCADAutils.version >= 5) {
                        msg.send(publisher.getOutputStream(), connectLog);   
                        CONNACKmsg cack = new CONNACKmsg(publisher.getInputStream());
                        if (cack.fixedHeader != (byte)0xE0)
                            throw new ScadaException("Did not get a DISCONNECT");
                    } else {
                        msg.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), connectLog, resultsLog);
                        throw new ScadaException("Didn't receive expected exception");
                    }    
                }
                // either SocketException or EOFException is acceptable here
                catch (SocketException e)
                {
                    resultsLog.println("Expected SocketException received - disconnected as expected");
                }
                catch (EOFException e)
                {
                    resultsLog.println("Expected EOFException received - disconnected as expected");
                }
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            
            /** *********************************************************** */
            // Simulate network instability by sending a partial packet 
            // (header only) and verifying other clients can still connect
            // and publications are received
            /** *********************************************************** */

            try
            {
                resultsLog.printTitle("TESTING RESEND PARTIAL PACKET");
                resultsLog.println("Creating publish message (header only)");
    
                pubOut = new PUBLISHmsg();
    
                resultsLog.println("Partial PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending partial PUBLISH message to broker...");
                resultsLog.println("OutputStream.isSocketClosed()="+publisher.isSocketClosed());
                resultsLog.println("OutputStream.isSocketConnected()="+publisher.isSocketConnected());
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Connect subscriber to broker
                resultsLog.println("Connecting subscriber");
                msg = new CONNECTmsg(subscriberClientId);
                msg.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), connectLog, resultsLog);
                
                // Subscribe to a topic
                resultsLog.println("Creating subscription list");
                subList = new SubList(topic1, qos0);
                subId = msgIds.getId();
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
                sub = new SUBSCRIBEmsg(subList, subId);
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
                
                // Reconnect publisher
                resultsLog.println("Reconnecting publisher");
                publisher = new Client(hostName, port);
                msg = new CONNECTmsg(publisherClientId);
                msg.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), connectLog, resultsLog);
                
                // Publish a full message
                resultsLog.println("Creating full publish message");
                pubOut = new PUBLISHmsg(topic1, payload1);
                resultsLog.println("Full PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending full PUBLISH message to broker...");
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, pubLog, resultsLog);
    
                // DISCONNECT msgs
                resultsLog.printTitle("DISCONNECT MESSAGES");
                resultsLog.println("Preparing DISCONNECT msg");
    
                disMsg = new DISCONNECTmsg();
    
                resultsLog.println("DISCONNECT message created in " + connectLog.name);
                resultsLog.println("Sending DISCONNECT message to broker...");
    
                resultsLog.println("Publisher OutputStream.isSocketClosed()="+publisher.isSocketClosed());
                resultsLog.println("Publisher OutputStream.isSocketConnected()="+publisher.isSocketConnected());
                resultsLog.println("Subscriber OutputStream.isSocketClosed()="+subscriber.isSocketClosed());
                resultsLog.println("Subscriber OutputStream.isSocketConnected()="+subscriber.isSocketConnected());
                disMsg.send(publisher.getOutputStream(), connectLog);
                resultsLog.println("Subscriber OutputStream.isSocketClosed()="+subscriber.isSocketClosed());
                resultsLog.println("Subscriber OutputStream.isSocketConnected()="+subscriber.isSocketConnected());
                disMsg.send(subscriber.getOutputStream(), connectLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            /** *********************************************************** */

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

