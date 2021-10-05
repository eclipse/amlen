/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.util.Random;


public class UDPSender {
    static String address = "10.10.0.5";
    static int    port = 9999;
    static int    count = 500;
    static int    rate  = 100;
    static int    topics = 10;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    public static void main(String [] args) throws Exception {
        String env;
        
        env = System.getenv("IMAServer");
        if (env != null) {
            address = env;
        }
        env = System.getenv("IMAPort");
        if (env != null) {
            port = Integer.parseInt(env);
        }
        if (port < 1 || port > 65535)
            port = 9999;
        env = System.getenv("IMACount");
        if (env != null) {
            count = Integer.parseInt(env);
        }
        if (count < 1)
            count = 1;
        env = System.getenv("IMARate");
        if (env != null) {
            count = Integer.parseInt(env);
        }
        if (rate < 1)
            rate = 1;
        env = System.getenv("IMATopics");
        if (env != null) {
            topics = Integer.parseInt(env);
        }
        if (topics < 1)
            topics = 1;
        
        double start = (double)System.currentTimeMillis();
        double permsg = 1000.0/rate;
        Random rnd = new Random();
        DatagramSocket sock = new DatagramSocket();
        

        try {
            SocketAddress saddr = new InetSocketAddress(address, port);
            System.out.println("Send UDP To=" + saddr + " Count=" + count + " Rate=" + rate);
            
            for (int i=0; i<count; i++) {
                int topicno = rnd.nextInt(topics);
                String msg = "MSG:0:topic/" + topicno + ":This is message "+ i;
                byte [] data = msg.getBytes();
                DatagramPacket pack = new DatagramPacket(data, data.length, saddr);
                sock.send(pack);
                double should = start + (i*permsg);
                double now = (double)System.currentTimeMillis();
                if (should-now > 2.0) {
                    try {
                        Thread.sleep((long)(should-now));
                    } catch (InterruptedException e) {
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace(System.err);
        }
        sock.close();
    }
    
}
