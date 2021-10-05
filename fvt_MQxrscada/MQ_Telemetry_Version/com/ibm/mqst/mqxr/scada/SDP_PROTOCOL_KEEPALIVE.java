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
import java.util.Date;

public class SDP_PROTOCOL_KEEPALIVE
{
    static final int numClients = 50;
    static Socket[]  activeClients = new Socket[numClients];

    public static void main(String[] args)
    {

        // Testcase
        String testName = "SDP_PROTOCOL_KEEPALIVE";
        String hostName = "localhost";

        int port = 1883;
        String arg;
        int i = 0;        

        // Logs
        String resultsLogName = testName + "_RESULTS.log";
        String connectLogName = testName + "_CONNECT.log";
        String pingLogName = testName + "_POLLING.log";
        String pubLogName = testName + "_PUBLISH.log";
        LogFile resultsLog = null;
        LogFile connectLog = null;
        LogFile pubLog = null;
        LogFile pingLog = null;

        // Socket variables
        Socket client1;
        DataOutputStream output1;
        DataInputStream input1;
        DataOutputStream output2;
        DataInputStream input2;

        
        // Messages
        CONNECTmsg connect1;
        CONNECTmsg connect2;
        PUBLISHmsg pubTmp; // Qos 1 - To verify disconnect
        PINGREQmsg ping;
        DISCONNECTmsg disMsg;

        MsgIdHandler msgIds;
        MsgIdentifier pubTmpId;

        // Test data
        String clientId1 = "C1KeepAlive";
        String clientId2 = "C2KeepAlive";      

        try
        {

            /** *********************************************************** */
            // SETUP AND CONNECT TO CLIENT1 - 30 SECOND KEEPALIVE
            /** *********************************************************** */

            resultsLog = new LogFile(resultsLogName);
            connectLog = new LogFile(connectLogName);
            pubLog     = new LogFile(pubLogName);
            pingLog    = new LogFile(pingLogName);
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
            
            pingLog.printTitle("Active client polling logged separately"+
                               " to avoid cluttering main logs");

            connectActiveClients(hostName, port, pingLog, resultsLog);

            // Create socket and setup data streams

            resultsLog.println("Creating sockets...");

            client1 = new Socket(hostName, port);

            resultsLog.println("Connected");

            client1.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            output1 = new DataOutputStream(client1.getOutputStream());
            input1 = new DataInputStream(client1.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER AS CLIENT1 - 30 SECOND KEEPALIVE");
            resultsLog.println("Creating CONNECT message");

            connect1 = new CONNECTmsg(clientId1);

            connect1.setKeepAlive((short) 30);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker");

            connect1.sendAndAcknowledge(input1, output1, connectLog, resultsLog);

            /** *********************************************************** */
            // WAIT FOR >30 SECS THEN VERIFY DISCONNECT
            /** *********************************************************** */

            resultsLog.printTitle("TESTING DISONNECT");

            resultsLog.println("Waiting for 60 seconds...");

            pingActiveClients(60000, pingLog, resultsLog);

            resultsLog.println("Trying publish at QoS1...");

            boolean exceptionThrown = false;

            try
            {

                pubTmpId = msgIds.getId();
                pubTmp = new PUBLISHmsg("JUNK", pubTmpId, "JUNK PAYLOAD".getBytes(), (short) 1);
                pubTmp.sendAndAcknowledge(input1, output1, pubLog, resultsLog);

            }
            // either SocketException or EOFException is acceptable here
            catch (SocketException e)
            {
                resultsLog.println("Expected SocketException received - DISCONNECT was recognised");
                exceptionThrown = true;
            }
            catch (EOFException e)
            {
                resultsLog.println("Expected EOFException received - DISCONNECT was recognised");
                exceptionThrown = true;
            }
            /** *********************************************************** */
            // SETUP AND CONNECT TO CLIENT2 - 30 SECOND KEEPALIVE + PING
            /** *********************************************************** */

            // Create socket and setup data streams
            resultsLog.println("Creating sockets...");

            client1 = new Socket(hostName, port);

            resultsLog.println("Connected");

            client1.setSoTimeout(SCADAutils.timeout);

            resultsLog.println("Socket timeout set to: " + SCADAutils.timeout);

            output2 = new DataOutputStream(client1.getOutputStream());
            input2 = new DataInputStream(client1.getInputStream());

            // Connect to broker and wait for reply

            resultsLog.printTitle("CONNECTING TO BROKER AS CLIENT2 - 30 SECOND KEEPALIVE");
            resultsLog.println("Creating CONNECT message");

            connect2 = new CONNECTmsg(clientId2);

            connect2.setKeepAlive((short) 30);

            resultsLog.println("Message created in " + connectLog.name);
            resultsLog.println("Sending CONNECT message to broker");

            connect2.sendAndAcknowledge(input2, output2, connectLog, resultsLog);

            /** *********************************************************** */
            // WAIT FOR 20 SECS THEN PING
            /** *********************************************************** */

            resultsLog.printTitle("WAITING THEN PINGING...");
            resultsLog.println("Waiting for 20 seconds...");

            pingActiveClients(20000, pingLog, resultsLog);

            resultsLog.println("Pinging broker...");

            ping = new PINGREQmsg();
            ping.sendAndAcknowledge(input2, output2, connectLog, resultsLog);

            /** *********************************************************** */
            // WAIT FOR 30 SECS THEN VERIFY CONNECTION
            /** *********************************************************** */

            resultsLog.printTitle("WAITING FOR 30 SECONDS AND VERIFYING CONNECTION");
            resultsLog.println("Waiting for 30 seconds...");

            pingActiveClients(30000, pingLog, resultsLog);

            resultsLog.println("Trying ping1...");

            ping.sendAndAcknowledge(input2, output2, connectLog, resultsLog);
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
 
            disconnectActiveClients(pingLog, resultsLog);

            resultsLog.printTitle("END OF TEST");

            /** *********************************************************** */
            // CATCH EXCEPTIONS
            /** *********************************************************** */

            // If none have been thrown, test has passed
            if (exceptionThrown == true)
            {
                resultsLog.println("TEST PASSED!");
                System.exit(0);
            }
            else
            {
                ScadaException myE = new ScadaException("Expected exception at disconnect was not thrown");
                throw myE;
            }

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

    // Create a group of client connections that can be pinged to keep 
    // the MQXR listener busy. Unless all the listeners threads are busy
    // servicing active connections, dormat connections will not be 
    // identified and disconnected
    private static void connectActiveClients(String hostName, 
                                             int port, 
                                             LogFile pingLog,
                                             LogFile resultsLog) 
    throws UnknownHostException, IOException, ScadaException
    {
      
        // Messages
        CONNECTmsg connect;
        String clientIdPrefix = "ActiveClient";

        DataOutputStream output;
        DataInputStream  input;

        resultsLog.println( "Connecting " + numClients + 
        " additional clients to ensure active monitoring of all connections.");

        for (int ac = 0; ac < numClients; ac++)
        {
           activeClients[ac] = new Socket(hostName, port);
           connect = new CONNECTmsg(clientIdPrefix+ac);
           output = new DataOutputStream( activeClients[ac].getOutputStream() );
           input = new DataInputStream( activeClients[ac].getInputStream() );
           connect.sendAndAcknowledge(input, output, pingLog, pingLog);
        }            

        resultsLog.println("Additional clients connected.");
    }

    // Poll the group of active clients with ping requests every second for the 
    // specified duration.
    private static void pingActiveClients(int requiredMillis, 
                                          LogFile pingLog, 
                                          LogFile resultsLog) 
    throws IOException
    {
        long startMillis = System.currentTimeMillis();

        PINGREQmsg       ping = new PINGREQmsg();
        PINGRESPmsg      pingReply;
        DataOutputStream output;
        DataInputStream  input;

        resultsLog.println("Pinging broker from active clients ...");
        do
        {
            for (int ac = 0; ac < numClients; ac++)
            {
                output = new DataOutputStream( activeClients[ac].getOutputStream() );
                input = new DataInputStream( activeClients[ac].getInputStream() );
                ping.send( output, pingLog );
                pingReply = new PINGRESPmsg( input );
            }                        
            SCADAutils.sleep(1000);
        } while ( System.currentTimeMillis() - startMillis < requiredMillis );
        resultsLog.println("Active client polling ended after " +
                           (System.currentTimeMillis()-startMillis) + 
                           " milliseconds.");
    }

    // Disconnect the group of active clients
    private static void disconnectActiveClients(LogFile pingLog, 
                                                LogFile resultsLog) 
    throws IOException
    {
        DISCONNECTmsg disMsg = new DISCONNECTmsg();
        DataOutputStream output;

        resultsLog.println("Disconnecting additional clients.");
        for (int ac = 0; ac < numClients; ac++)
        {
            output = new DataOutputStream( activeClients[ac].getOutputStream() );
            disMsg.send( output, pingLog );
        }
        resultsLog.println(numClients + " additional clients disconnected.");
    }
}
