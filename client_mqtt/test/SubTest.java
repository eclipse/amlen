/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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


public class SubTest implements IsmMqttListener {

    static boolean closed = false;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@defg");
        conn.setEncoding(IsmMqttConnection.BINARY);
        conn.setListener(new SubTest());
        conn.setVerbose(true);
        conn.setClean(true);
        conn.connect();
        System.out.println("connected");
        byte[] ret = conn.subscribe(new String [] {"/fred/abc", "$SYS/Bad", "mytopic"}, new int [] {2, 2, 128});
        System.out.println("subscribe: " + ret[0] + " " + ret[1] + " " + ret[2]);
        conn.publish("/fred/abc", "This is a message");
        try { Thread.sleep(10000); } catch (InterruptedException ex) { }
        closed = true;
        conn.disconnect();
    }

    public void onDisconnect(int rc, Throwable e) {
        System.out.println("Disconnect: rc="+rc);
        if (e != null && !closed) { 
            e.printStackTrace(System.out);
            System.exit(1);
        }           
            
    }

    public void onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
    }
    
}

