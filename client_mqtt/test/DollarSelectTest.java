/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
import com.ibm.ism.mqtt.IsmMqttConnection;

import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;


public class DollarSelectTest implements IsmMqttListener {

    static volatile boolean closed = false;
    final static boolean clean = false;
    final static boolean reset = false;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn;
        
        if (reset) {
            conn = new IsmMqttConnection("kwb5", 16109, "/mqtt", "abc#def");
            conn.setEncoding(IsmMqttConnection.TCP4);
            conn.setListener(new DollarSelectTest());
            conn.setSecure(0);
            conn.setClean(false);
            conn.setUser("kwb", "password");
            conn.connect();
            conn.disconnect();
            System.out.println("reset");    
        }
        conn = new IsmMqttConnection("kwb5", 16109, "/mqtt", "abc#def");
        conn.setEncoding(IsmMqttConnection.TCP4);
        conn.setListener(new DollarSelectTest());
        conn.setSecure(0);
        conn.setClean(clean);
        conn.setUser("kwb", "password");
        conn.connect();
        System.out.println("connect");
        conn.subscribe("$select/QoS=0/fred/sam", 0);
        conn.subscribe("$select/QoS>0/fred/sams", 0);
        conn.subscribe("$select/QoS=0/$SharedSubscription/mysub/fred/sam", 0);
        conn.subscribe("$select/QoS>0/$SharedSubscription/mysub2/fred/sam", 0);
        conn.subscribe("$select/QoS=0/$share/mine/fred/sam2", 0);
        conn.subscribe("$select/QoS>0/$share/mine2/fred/sams", 0);
        System.out.println("complete subscribe 1");
        
        if (clean) {
            IsmMqttConnection conn1 = new IsmMqttConnection("kwb5", 16109, "/mqtt", "abc#deg");
            conn1.setEncoding(IsmMqttConnection.TCP4);
            conn1.setListener(new DollarSelectTest());
            conn1.setSecure(0);
            conn1.setClean(true);
            conn1.setUser("kwb", "password");
            conn1.connect();
            System.out.println("connect");
            conn1.subscribe("$select/QoS=0/fred/sam", 0);
            conn1.subscribe("$select/QoS>0/fred/sams", 0);
            conn1.subscribe("$select/QoS=0/$SharedSubscription/mysub/fred/sam", 0);
            conn1.subscribe("$select/QoS>0/$SharedSubscription/mysub2/fred/sam", 0);
            conn1.subscribe("$select/QoS=0/$share/mine/fred/sam2", 0);
            conn1.subscribe("$select/QoS>0/$share/mine2/fred/sams", 0);
            System.out.println("complete subscribe 2");
            conn1.disconnect();
        }
        
        conn.disconnect();
        System.out.println("after disconnect");
    }
  

    public void onDisconnect(int rc, Throwable e) {
        System.out.println("Disconnect: rc="+rc+" closed=" + closed);
        if (e != null && !closed) { 
            e.printStackTrace(System.out);
            System.exit(1);
        }  
    }    
                    
        

    public void onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" retain="+(msg.getRetain()?'1':'0')+" body=\""+msg.getPayload()+"\"");
    }
}

