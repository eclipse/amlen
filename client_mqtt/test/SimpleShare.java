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
import java.io.IOException;

import com.ibm.ism.mqtt.IsmMqttConnection;

import com.ibm.ism.mqtt.IsmMqttMessage;

public class SimpleShare {
    
    static volatile boolean closed = false;

    public static void main(String [] args) throws IOException {
        int  rc;
        IsmMqttConnection conn1 = new IsmMqttConnection("kwb5", 16109, "/mqtt", "myclient");
        conn1.setEncoding(IsmMqttConnection.TCP4);
        conn1.setListener(new IoTTest());
        conn1.setVerbose(true);
        conn1.setClean(false);
        System.out.println("conn1="+conn1);
        conn1.connect();
        System.out.println("conn1 connected");
        rc = conn1.subscribe("$SharedSubscription/fred/sam", 0);
        System.out.println("subscribe " + rc);
        conn1.disconnect();
        System.out.println("Disconnect conn1");
        
        IsmMqttConnection conn2 = new IsmMqttConnection("kwb5", 16109, "/mqtt", "myclient");
        conn2.setEncoding(IsmMqttConnection.TCP4);
        conn2.setListener(new IoTTest());
        conn2.setVerbose(true);
        conn2.setClean(false);
        conn2.setUser("kwb", "password");
        System.out.println("conn2="+conn2);
        conn2.connect();
        rc = conn2.subscribe("$SharedSubscription/fred/sam", 0);
        System.out.println("subscribe " + rc);
        conn2.unsubscribe("$SharedSubscription/fred/sam");
        closed = true;
        conn2.disconnect();
        System.out.println("Disconnect conn2");
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
