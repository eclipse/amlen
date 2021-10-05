/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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


import java.io.IOException;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;

/*
 * Test MQTT send and receive
 * 
 * There are four positional arguments which can be set when running this test:
 * 1. The number of messages to send from each sender (default = 100)
 * 2. The name of the topic (default = "RequestT")
 * 3. The number of senders on the topic (default = 1)
 * 4. The number of receivers on the topic (default = 1)
 * 
 * The location of the ISM server can be specified using two environment variables:
 * ISMServer - The IP address of the server (default = 127.0.0.1)
 * ISMPort   - The port MQTT number of the server (default = 16102)
 * 
 * The default can also be modified in the section below.
 */
public class ClusterTest implements Runnable {
    static final int SENDER = 1;
    static final int RECEIVER = 2;
    static int  use_qos = 0;
        
    static int  count = 1004;  //Messages published per publisher
    static int  servers = 2;
    static String topicName = "RequestT";
    
    static IsmMqttConnection[]        sendconn;
    static IsmMqttConnection[]        recvconn;
    
    static volatile boolean[] recvdone;
    static volatile boolean  [] senddone;
    
    static int []  sendcount;
    static int []  recvcount;
    
    int insttype = 0;     // Sender or receiver
    int senderInst = 0;   // Which sender does this class represent
    int receiverInst = 0; // Which receiver does this class represent
    IsmMqttConnection conn;
    
    /*
     * Wait for all the senders and receivers to finish
     */
    void waitForMessagesReceived() {
        int wait = 15000;           
        boolean allDone = false;
        
        while (wait-- > 0 && !allDone) {
            allDone = true;
            //Check whether all 
            for(int i = 0; i < senddone.length; i++) {
                if(!senddone[i]) {
                    allDone = false;
                    break;
                }
            }
            if (allDone) {
                for(int i = 0; i < recvdone.length; i++) {
                    if(!recvdone[i]) {
                        allDone = false;
                        break;
                    }
                }
            }
            if (!allDone) {
                try { Thread.sleep(50); } catch (Exception e) {}
            }
        }
        for (int i=0; i<recvdone.length; i++) {
            if (!recvdone[i]) {
                System.out.println("Recv("+i+") - Messages were not all received");
            }
        }
    }
    
    /*
     * Send messages
     */
    public long doSend() throws IOException {       
        try { Thread.sleep(100); } catch (Exception e) {}
        
        long startTime = System.currentTimeMillis();
        
        for (int i=0; i<count; i++) {
            conn.publish(topicName, "This is message "+i, use_qos, false);
            sendcount[senderInst]++;
            if (sendcount[senderInst]%500 == 0)
                try { Thread.sleep(3);} catch (Exception e) {} 
        }
        
        senddone[senderInst] = true;
        System.out.println("Done sender "+senderInst);
        
        if(senderInst == 0) {
            //We're running on the main thread and we'll wait for all the other senders and
            //receivers to finish
            waitForMessagesReceived();
        }
        long endTime = System.currentTimeMillis();
        return endTime-startTime;   
    }
    
    public ClusterTest(int insttype, int inst, IsmMqttConnection conn) {
        this.insttype = insttype;
        if(insttype == SENDER)
            this.senderInst = inst;
        else // InstType == RECEIVER
            this.receiverInst = inst;
        this.conn = conn;
    }
    
    public void run() {
        try {
            if(insttype == SENDER)
                doSend();
        } catch (IOException e) {
            e.printStackTrace();
        }       
    }

    /*
     * Parse arguments
     */
    public static void parseArgs(String [] args) {
        if (args.length > 0) {
            try { 
                count = Integer.valueOf(args[0]);
            } catch (Exception e) {
                syntaxhelp("");
                return;
            }
        }
        if (args.length > 1) {
            try {
                servers = Integer.valueOf(args[1]);
            } catch (Exception e) {
                syntaxhelp("");
                return;
            }    
        }

        if (args.length > 2) {
            syntaxhelp("Too many command line arguments");
        }
    }

    
    /*
     * Put out a simple syntax help
     */
    public static void syntaxhelp(String msg)  {
        if (msg != null)
            System.out.println(msg);
        System.out.println("ClusterTest  count [numProducers] [numConsumers] ");
    }
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
        int   i;
        long millis = 0;
        
        parseArgs(args);

        int port;
        
        String server = System.getenv("IMAServer");
        String portstr   = System.getenv("IMAPort");
        String protocol   = "mqtt";
        if (server == null) 
            server = "127.0.0.1";
        if (portstr == null) 
            port = 16109;
        else
            port = Integer.parseInt(portstr);
        
        String cserver [] = new String[servers];
        int    cport   [] = new int[servers];
        
        for (i=0; i<servers; i++) {
            String ip = System.getenv("IMAServer"+(i+1));
            String pp = System.getenv("IMAPort"+(i+1));
            // System.out.println("i="+ i + " ip="+ip + " port=" + pp);
            if (ip == null)
                ip = server;
            cserver[i] = ip;
            if (pp == null) {
                cport[i] = port;    
            } else {
                cport[i] = Integer.parseInt(pp);
            }
        }

           
        /*Setup the arrays that depend on the number of consumers */
        recvcount = new int[servers];
        recvdone = new boolean[servers];
        recvconn = new IsmMqttConnection[servers];
        
        /*Setup the arrays that depend on the number of producers */
        senddone = new boolean[servers];
        sendcount = new int[servers];
        sendconn = new IsmMqttConnection[servers];     
        
        /*
         * Create connections
         */
        for (i = 0; i < servers; i++) { 
            System.out.println("create semder" + (i+1) + ": Server="+cserver[i] + "Port=" + cport[i]);
            sendconn[i] = new IsmMqttConnection(cserver[i], cport[i], "/"+protocol, "clusterSend"+(i+1));
            sendconn[i].setEncoding(IsmMqttConnection.BINARY4);
            sendconn[i].setVerbose(false);
        }
        for (i = 0; i < servers; i++) {
            System.out.println("create receiver" + (i+1) + ": Server="+cserver[i] + " Port=" + cport[i]);
            recvconn[i] = new IsmMqttConnection(cserver[i], cport[i], "/"+protocol, "clusterReceive"+(i+1));
            recvconn[i].setEncoding(IsmMqttConnection.BINARY4);
            recvconn[i].setVerbose(false);
        }
        
        /*
         * Start the connections
         */
        try {
            for (i=0; i < servers; i++) {
                System.out.println("connect sender " + (i+1) + ": Server="+cserver[i] + " Port=" + cport[i]);
                sendconn[i].connect();
            }
            for(i=0; i < servers; i++) {
                System.out.println("connect receiver " + (i+1) + ": Server="+cserver[i] + " Port=" + cport[i]);
                ClusterTest recvInst = new ClusterTest(RECEIVER, i, recvconn[i]);
                ClusterListener listener = recvInst.new ClusterListener(i);
                recvconn[i].setListener(listener);
                recvconn[i].connect();
                recvconn[i].subscribe(topicName, 0);  
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        try {
            Thread.sleep(400);
        } catch (InterruptedException e1) {
            e1.printStackTrace();
        }
        
        /*
         * If we have multiple senders, start threads to
         * do all but one send...
         */
        for (i=1; i<servers; i++) {
            new Thread(new ClusterTest(SENDER, i, sendconn[i])).start();
        }
        
        
        /* Run a sender on the main thread... */
        try {
            ClusterTest t = new ClusterTest(SENDER, 0, sendconn[0]);
            millis = t.doSend();
        } catch (Exception ex) {
            ex.printStackTrace(System.out);
        }
        
        /*
         * Print out statistics
         */
        System.out.println("Time = "+millis);
        for (i = 0; i < sendcount.length; i++) {
            System.out.println("Send("+i+") = "+sendcount[i]);
        }
        for (i = 0; i < recvcount.length; i++) {
            System.out.println("Recv("+i+") = "+recvcount[i]);
        }
        
        try {
            for (i=0; i < servers; i++) {
                sendconn[i].disconnect();
            }
            for(i=0; i < servers; i++) {
                recvconn[i].unsubscribe(topicName);
                recvconn[i].disconnect();
            }
        } catch (Exception e) {
            //e.printStackTrace();
        }
    }
    
    class ClusterListener implements IsmMqttListener {
        int inst;
        
        ClusterListener(int inst) {
            this.inst = inst;
        }

        public void onDisconnect(int rc, Throwable e) {
            System.out.println("Disconnect: rc="+rc);
            if (e != null) 
                e.printStackTrace(System.out);
                
        }
        
        public void onMessage(IsmMqttMessage msg) {          
            String msgstr = msg.getPayload();
            
            /* Validate the message */
            //If there are multiple producers we can't tell which message we should be receiving
            //So only check in the case there is 1 producer
            if (servers == 1) {
                if (!msgstr.equals("This is message "+recvcount[receiverInst])) {
                    throw new RuntimeException("Message received is not the correct message: expected(This is message "+recvcount[receiverInst]+") found("+msgstr+")");
                    //System.out.println("Message received is not the correct message: expected(This is message "+recvcount[receiverInst]+") found("+msgstr+")");
                }
            }

            recvcount[receiverInst]++;
            if (recvcount[receiverInst] == servers * count) {
                recvdone[receiverInst] = true;
                System.out.println("Receiver "+receiverInst+" complete");
            }
        }
    }
}

