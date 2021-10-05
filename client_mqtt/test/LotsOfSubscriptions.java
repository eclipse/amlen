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

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;


public class LotsOfSubscriptions implements IsmMqttListener {

    static boolean closed = false;
    
    static int i;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "kwb");
        conn.setEncoding(IsmMqttConnection.BINARY);
        conn.setListener(new LotsOfSubscriptions());
        conn.setVerbose(true);
        conn.connect();
        try {
            for (i=0; i<400; i++) {
                conn.subscribe("fred"+i, 0);
            }    
            for (i=0; i<100; i++) {
                conn.subscribe("fred"+i, 0);
            }
            for (i=0; i<100; i++) {
                conn.unsubscribe("fred"+(i+100));
            }
            for (i=400; i<600; i++) {
                conn.subscribe("fred"+i, 0);
            } 
        } catch (Exception e) {
            System.out.println(""+e+ " i="+ i);
        }
        conn.disconnect();
    }

    public void onDisconnect(int rc, Throwable e) {
        System.out.println("Count = " + i);
        System.out.println("Disconnect: rc="+rc);
        if (e != null && !closed) 
            e.printStackTrace(System.out);
            
    }

    public void onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
    }
    
}
