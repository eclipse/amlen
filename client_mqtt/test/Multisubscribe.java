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


public class Multisubscribe implements IsmMqttListener {

    static boolean closed = false;
    
    static int i;
    static int count = 200;
    static int qosin = 2;
    static int encoding = IsmMqttConnection.TCP5;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "kwb");
        conn.setEncoding(encoding);
        conn.setListener(new Multisubscribe());
        conn.setVerbose(true);
        conn.connect();
        String [] topic = new String[count];
        String [] not_topic = new String[count]; 
        int [] qos = new int[count];
        byte [] ret = null;
        int expected = encoding<IsmMqttConnection.BINARY4 ? 0 : 128; 
        System.out.println("encoding="+encoding+" expected="+expected);
        
        try {
            for (i=0; i<count; i++) {
                topic[i] = "topic"+i;
                not_topic[i] = "not_topic" + i;
                qos[i] = qosin;
            }    
            long starttime = System.currentTimeMillis();
            ret = conn.subscribe(topic, qos);
            long endtime = System.currentTimeMillis();
            System.out.println("Count="+count+ " Time=" + (double)(endtime-starttime)/1000 + " Per=" 
                    + (double)count/(endtime-starttime));
            for (i=0; i<count; i++) {
                if ((ret[i]&0xff) != qosin) {
                    System.out.println("Subscribe return incorrect " + i + ": " + (ret[i]&0xff));
                    for (i++; i<count; i++) {
                        if ((ret[i]&0xff) != expected) 
                            System.out.println("Subscribe return incorrect " + i + ": " + (ret[i]&0xff));
                    }
                }    
            } 
            System.out.println(""+conn.unsubscribe(not_topic, true));
            System.out.println(""+conn.unsubscribe(topic, true));
        } catch (Exception e) {
            System.out.println(""+e+ " i="+ i);
            e.printStackTrace(System.out);
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
