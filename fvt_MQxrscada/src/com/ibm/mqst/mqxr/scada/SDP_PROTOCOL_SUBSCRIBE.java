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

import java.io.EOFException;
import java.net.SocketException;
import java.net.SocketTimeoutException;

public class SDP_PROTOCOL_SUBSCRIBE
{

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_SUBSCRIBE";
        String hostName = "localhost";
        boolean testPassed = true;
        int port = 1883;
        int altport = 16103;
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
        UNSUBSCRIBEmsg unsub;
        PUBLISHmsg pubOut;
        PUBLISHmsg pubIn;
        PUBLISHmsg pubInEX;
        DISCONNECTmsg disMsg;

        // Support classes
        Client subscriber;
        Client publisher;
        Client altsubscriber;
        MsgIdHandler msgIds;
        SubList subList;
        TopicList topicList;
        MsgIdentifier msgId;

        // Test data
        String subscriberClientId = "C1SubscribeSub";
        String altsubscriberClientId = "C1AltSubscribeSub";
        String publisherClientId = "C2SubscribePub";
       
        String topic1 = "SubscribeTopic1";
        String topic2 = "SubscribeTopic2";
        String topic3 = "SubscribeTopic3";
        String topic4 = "SubscribeTopic4";
        byte[] payload1 = "This is payload 1".getBytes();
        byte[] payload2 = "This is payload 2".getBytes();
        byte qos0 = 0;
        byte qos1 = 1;
        byte qos2 = 2;

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
                else if (arg.equals("-altport") || arg.equals("-a")) {
                    if (i < args.length) {
                        try {
                            altport = Integer.parseInt(args[i++]);
                        }
                        catch (NumberFormatException nfe) {
                            resultsLog.println("Arguments error: altport requires a number");                          
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
            // SUBSCRIBE to an invalid topic
            /** *********************************************************** */
            if (SCADAutils.version >= 4) {
                SUBACKmsg subAckEX = null;
                sub = null;
                try
                {
                    resultsLog.printTitle("TESTING SUBSCRIBING TO " + topic4);
                    altsubscriber = new Client(hostName, altport);
                    resultsLog.println("Connecting altsubscriber");
                    msg = new CONNECTmsg(altsubscriberClientId);
                    msg.sendAndAcknowledge(altsubscriber.getInputStream(), altsubscriber.getOutputStream(), connectLog, resultsLog);
                    
                    // Create subscription list and subscribe
                    subList = new SubList(topic4, qos0);
                    msgId = msgIds.getId();
                    resultsLog.println("msgId is: "+(((0xff&(int)msgId.msgId[0])*256)+(0xff&(int)msgId.msgId[1])));
        
                    resultsLog.println("Subscriptions prepared");
                    resultsLog.println("Creating subscription message");
        
                    sub = new SUBSCRIBEmsg(subList, msgId);
        
                    resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                    resultsLog.println("Sending SUBSCRIBE message to broker...");
        
                    subAckEX = new SUBACKmsg(subList, sub.myId);
                    subAckEX.grantedQOS[0] = (byte)0x80;
                    if (SCADAutils.version >= 5)
                        subAckEX.grantedQOS[0] = (byte)0x87;  /* Not authorized */
                    else 
                        subAckEX.grantedQOS[0] = (byte)0x80;  /* Error */
                    subAckEX.rebuild();
                    resultsLog.println("AltSubscriber OutputStream.isSocketClosed()="+altsubscriber.isSocketClosed());
                    resultsLog.println("AltSubscriber OutputStream.isSocketConnected()="+altsubscriber.isSocketConnected());
                    sub.sendAndAcknowledge(altsubscriber.getInputStream(), altsubscriber.getOutputStream(), subAckEX, subLog, resultsLog);
                }
                catch (Exception e)
                {
                    /*
                     * This seems to fail occasionally, so we will retry before failing                                                        
                     */
                    if (sub != null && subAckEX != null) {
                        try {
                            resultsLog.println("Connecting to altsubscriber again");
                            altsubscriber = new Client(hostName, altport);
                            msg.sendAndAcknowledge(altsubscriber.getInputStream(), altsubscriber.getOutputStream(), connectLog, resultsLog);
                            sub.sendAndAcknowledge(altsubscriber.getInputStream(), altsubscriber.getOutputStream(), subAckEX, subLog, resultsLog);
                        } catch (Exception ex) {
                            e.printStackTrace(resultsLog.out);
                            testPassed = false;
                        }   
                    } else {
                        e.printStackTrace(resultsLog.out);
                        testPassed = false;
                    }
                }
            }
            
            /** *********************************************************** */
            // SUBSCRIBE to topic1/#
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBING TO " + topic1 + "/#");
    
                subList = new SubList(topic1 + "/#", qos0);
                msgId = msgIds.getId();
                resultsLog.println("msgId is: "+(((0xff&(int)msgId.msgId[0])*256)+(0xff&(int)msgId.msgId[1])));
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1);
    
                pubOut = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                
                // PUBLISH msg to topic1/topic2
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // PUBLISH msg to topic1/topic2/topic3
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2 + "/" + topic3);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2 + "/" + topic3, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1 + "/" + topic2 + "/" + topic3, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // UNSUBSCRIBE from topic1/#
                topicList = new TopicList(topic1 + "/#");
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1 + "/#");
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
                

            /** *********************************************************** */
            // SUBSCRIBE to topic1/topic2
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBING TO " + topic1 + "/" + topic2);
    
                subList = new SubList(topic1 + "/" + topic2, qos0);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1);
    
                pubOut = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message available");
                }
                
                
                // PUBLISH msg to topic1/topic2
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                
                // PUBLISH msg to topic1/topic2/topic3
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2 + "/" + topic3);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2 + "/" + topic3, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message available");
                }
                
                // UNSUBSCRIBE from topic1/topic2
                topicList = new TopicList(topic1 + "/" + topic2);
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1 + "/" + topic2);
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // SUBSCRIBE to topic1/+
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBING TO " + topic1 + "/+");
    
                subList = new SubList(topic1 + "/+", qos0);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1);
    
                pubOut = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message available");
                }
                
                // PUBLISH msg to topic1/topic2
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // PUBLISH msg to topic1/topic2/topic3
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2 + "/" + topic3);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2 + "/" + topic3, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message available");
                }
                
                // UNSUBSCRIBE from topic1/+
                topicList = new TopicList(topic1 + "/+");
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1 + "/+");
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // SUBSCRIBE to topic1
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBING TO " + topic1);
    
                subList = new SubList(topic1, qos0);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1);
    
                pubOut = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // PUBLISH msg to topic1/topic2
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message available");
                }
                
                // UNSUBSCRIBE from topic1
                topicList = new TopicList(topic1);
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1);
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // SUBSCRIBE to +
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
            	String topicWildcard = "+";
                resultsLog.printTitle("TESTING SUBSCRIBING TO " + topicWildcard);
    
                subList = new SubList(topicWildcard, qos0);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1);
    
                pubOut = new PUBLISHmsg(topic1, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // PUBLISH msg to topic1/topic2
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message available");
                }
                
                // UNSUBSCRIBE from +
                topicList = new TopicList(topicWildcard);
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topicWildcard);
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // Subscribe to multiple topics at different QOS levels and check SUBACK
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBING TO MULTIPLE TOPICS AT DIFFERENT QOS LEVELS, CHECKING SUBACKS");
                resultsLog.println("Subscribing to " + topic1 + " at QOS 0");
                resultsLog.println("Subscribing to " + topic2 + " at QOS 1");
                resultsLog.println("Subscribing to " + topic3 + " at QOS 2");
    
                subList = new SubList(topic1, qos0);
                subList.addSub(topic2, qos1);
                subList.addSub(topic3, qos2);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                SUBACKmsg subAckEX = new SUBACKmsg(subList, sub.myId);
                subAckEX.grantedQOS[0] = (byte)0x80;
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subAckEX, subLog, resultsLog);
                
                // UNSUBSCRIBE from topics
                topicList = new TopicList(topic1);
                topicList.addTopic(topic2);
                topicList.addTopic(topic3);
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1 + ", " + topic2 + ", " + topic3);
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // Subscribe to subtopic, unsubscribe to root topic and #. 
            // Should stay subscribed to subtopic
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBING TO SUBTOPIC AND UNSUBSCRIBING FROM ROOT TOPIC");
                resultsLog.println("Subscribing to " + topic1 + "/" + topic2);
    
                subList = new SubList(topic1 + "/" + topic2, qos0);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // UNSUBSCRIBE from topic1
                topicList = new TopicList(topic1);
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1);
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                if (SCADAutils.version >= 5)
                    unsub.expectedRc = 17; 
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // UNSUBSCRIBE from #
                topicList = new TopicList("#");
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for #");
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
                
                if (SCADAutils.version >= 5)
                    unsub.expectedRc = 17; 
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1 + "/" + topic2);
    
                pubOut = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
    
                resultsLog.println("PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.send(publisher.getOutputStream(), pubLog);
    
                // Should now get the PUBLISH msg back - we are subscribed
                resultsLog.println("Waiting for published data");
                pubInEX = new PUBLISHmsg(topic1 + "/" + topic2, payload1);
                pubIn = new PUBLISHmsg(subscriber.getInputStream());
                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);
                
                // UNSUBSCRIBE from topic1/topic2
                topicList = new TopicList(topic1 + "/" + topic2);
                msgId = msgIds.getId();
    
                resultsLog.println("Preparing UNSUBSCRIBE msg for " + topic1 + "/" + topic2);
    
                unsub = new UNSUBSCRIBEmsg(topicList, msgId);
    
                resultsLog.println("UNSUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending UNSUBSCRIBE message to broker...");
    
                unsub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }


            /** *********************************************************** */
            // Subscribe, disconnect, connect- check retained pubs are received
            /** *********************************************************** */
            try
            {
                // Create subscription list and subscribe
                resultsLog.printTitle("TESTING SUBSCRIBE, DISCONNECT, CONNECT- CHECK RETAINED PUBS ARE RECEIVED");

                // DISCONNECT subscriber
                resultsLog.println("Reconnecting subscriber with CLEANSTART bit clear");
                disMsg = new DISCONNECTmsg();
                resultsLog.println("DISCONNECT message created in " + connectLog.name);
                resultsLog.println("Sending DISCONNECT message to broker...");
                disMsg.send(subscriber.getOutputStream(), connectLog);
                subscriber = new Client(hostName, port);
                msg = new CONNECTmsg(subscriberClientId, false);
                msg.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), connectLog, resultsLog);

                resultsLog.println("Subscribing to " + topic1);
    
                subList = new SubList(topic1, qos1);
                msgId = msgIds.getId();
    
                resultsLog.println("Subscriptions prepared");
                resultsLog.println("Creating subscription message");
    
                sub = new SUBSCRIBEmsg(subList, msgId);
    
                resultsLog.println("SUBSCRIBE message created in " + subLog.name);
                resultsLog.println("Sending SUBSCRIBE message to broker...");
    
                sub.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), subLog, resultsLog);
    
                // DISCONNECT subscriber
                resultsLog.println("Preparing DISCONNECT msg");
    
                disMsg = new DISCONNECTmsg();
                
                resultsLog.println("DISCONNECT message created in " + connectLog.name);
                resultsLog.println("Sending DISCONNECT message to broker...");
    
                disMsg.send(subscriber.getOutputStream(), connectLog);
                
                // Sleep for 1 second to sort out timing issues with DISCONNECT message. 
                // We are not happy about this but best we can do for the time being. 
                Thread.sleep(1000);
    
                // PUBLISH msg to topic1
                resultsLog.println("Creating publication message - " + topic1);
    
                msgId = msgIds.getId();
                pubOut = new PUBLISHmsg(topic1, msgId, payload2, qos1);
                pubOut.setRetain();
                
                resultsLog.println("Retained PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                pubOut.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), pubLog, resultsLog);
    
                // Should NOT get a PUBLISH msg back
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    throw new ScadaException("Didn't get expected exception");
                }
                // either SocketException or EOFException is acceptable here
                catch (SocketException e)
                {
                    resultsLog.println("Expected SocketException received - no message received as expected");
                }
                catch (EOFException e)
                {
                    resultsLog.println("Expected EOFException received - no message received as expected");
                }
    
                // RECONNECT subscriber
                resultsLog.println("Reconnecting subscriber");
                subscriber = new Client(hostName, port);
                msg = new CONNECTmsg(subscriberClientId, false);
                // We may get the retained message PUBLISH before we get the CONNACK
                // So we need to read the message, and if it is the CONNACK, then compare it. 
                // If not, read the PUBLISH and save it and then read the CONNACK and compare it.
                msg.send(subscriber.getOutputStream(), connectLog);

                resultsLog.println("Message sent - waiting for reply");

                CONNACKmsg reply = new CONNACKmsg(subscriber.getInputStream());
                boolean gotPUBLISHmsg = false;
                pubIn = null;
                if (null != reply.realMsg && PUBLISHmsg.MYTYPE == reply.realMsg.messageType) {
                	gotPUBLISHmsg = true;
                	pubIn = (PUBLISHmsg)reply.realMsg;
                	reply = new CONNACKmsg(subscriber.getInputStream());
                }
                CONNACKmsg replyEX = new CONNACKmsg();
               	replyEX.setSessionPresent(true);
                msg.validateAck(reply, replyEX, connectLog, resultsLog);
                //msg.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream(), connectLog, resultsLog);  

                // Should now get the retained PUBLISH msg back from durable
                // subscription
                resultsLog.printTitle("GETTING RETAINED PUBLISH MESSAGES");

                // Expected PUBLISH msg (Qos1)
                pubInEX = new PUBLISHmsg(topic1, msgId, payload2, qos1);

                // Do not expect the retain flag to be set. Even though this 
                // client was not connected when the message was published, the
                // message could be delivered directly to the existing durable 
                // subscription and so it did not have to be retained.
                // pubInEX.setRetain();

                if (!gotPUBLISHmsg) {
                	pubIn = new PUBLISHmsg(subscriber.getInputStream());
                }
                pubInEX.setId(pubIn.myId);

                SCADAutils.compareAndLog(pubIn, pubInEX, subLog, resultsLog);

                // DISCONNECT subscriber
                resultsLog.println("Disconnecting client");
    
                disMsg = new DISCONNECTmsg();
    
                resultsLog.println("DISCONNECT message created in " + connectLog.name);
                resultsLog.println("Sending DISCONNECT message to broker...");
    
                disMsg.send(subscriber.getOutputStream(), connectLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }
            
            
            /** *********************************************************** */
            // Publish, connect client with cleanstart- check no pubs are received
            /** *********************************************************** */
            try
            {
                // PUBLISH retained message
                resultsLog.println("Creating publication message - " + topic1);
    
                msgId = msgIds.getId();
                pubOut = new PUBLISHmsg(topic1, msgId, payload2, qos1);

                pubOut.setRetain();
                
                resultsLog.println("Retained PUBLISH message created in " + pubLog.name);
                resultsLog.println("Sending PUBLISH message to broker...");
    
                // Get acknowledgement for publish before attempting to reconnect the subscriber
                // client. Otherwise the clean start processing might complete before the message
                // has been successfully published.
                pubOut.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), 
                		                  pubLog, resultsLog);
    
                // CONNECT subscriber
                resultsLog.println("Reconnecting subscriber with CLEANSTART");
                subscriber = new Client(hostName, port);
                msg = new CONNECTmsg(subscriberClientId, true);
                msg.sendAndAcknowledge(subscriber.getInputStream(), subscriber.getOutputStream()
                		, connectLog, resultsLog);
    
                resultsLog.println("Waiting to check no PUBLISH message is received");
                try
                {
                    pubIn = new PUBLISHmsg(subscriber.getInputStream());
                    resultsLog.println("Received unexpected message of type: "+pubIn.getMessageType());
                    pubIn.parseAndPrint(pubLog);
                    throw new ScadaException("Didn't get expected exception");
                }
                catch (SocketTimeoutException e)
                {
                    resultsLog.println("Expected SocketTimeoutException received - no message received as expected");
                }
                
                // Now send a 0 length RETAIN message to delete the RETAINed message
                resultsLog.println("Try to delete RETAINed message");
                pubOut = new PUBLISHmsg(topic1, msgId, "".getBytes(), qos1);
                pubOut.setNoSubscription(true);
                pubOut.setRetain();
                
                pubOut.sendAndAcknowledge(publisher.getInputStream(), publisher.getOutputStream(), pubLog, resultsLog);
            }
            catch (Exception e)
            {
                e.printStackTrace(resultsLog.out);
                testPassed = false;
            }

            
            /** *********************************************************** */

            // DISCONNECT msgs
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
            System.exit(testPassed ? 0 : 1);
        }

        /** *********************************************************** */
        // END TEST
        /** *********************************************************** */

    }
}


