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

import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;


public class SimpleIoT implements IsmMqttListenerAsync {
    static int sleeptries = 200;
    static volatile boolean closed = false;
    static volatile int     received = 0;
    static int count = 2000;
    static int qos = 2;
    
    public static void main(String [] args) throws Exception {
        //IsmMqttConnection conn = new IsmMqttConnection("quickstart.messaging.internetofthings.ibmcloud.com", 8883, "/mqtt", "a:quickstart:devicekwb99");
        // IsmMqttConnection conn = new IsmMqttConnection("quickstart.messaging.wdc01-1.test.internetofthings.ibmcloud.com", 8883, "/mqtt", "a:quickstart:devicekwb99");
        IsmMqttConnection conn = new IsmMqttConnection("kwb5", 1883, "/mqtt", "a:quickstart:devicekwb99");
        conn.setEncoding(IsmMqttConnection.WS5);
        conn.setAsyncListener(new SimpleIoT());
        conn.setSecure(0);
        conn.setTopicAlias(5, 5);
        conn.setMaxPacketSize(5000);
        conn.connect();
        System.out.println("connect");
        conn.subscribe("iot-2/type/typ2/id/devicekwb99/evt/fred/fmt/json", 1);
        System.out.println("subscribe");
        String t = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";   /* 100 */
        t = t + t + t + t + t;  /* 500 */
        // t = t + t +t + t + t;   /* 2500 */
        for (int i=0; i<count; i++) {
            IsmMqttMessage msg = new IsmMqttMessage("iot-2/type/typ2/id/devicekwb99/evt/fred/fmt/json", 
                    "This is a message " + i + " " + t, qos, false);
            msg.addUserProperty("Prop1", ""+i);
            msg.setContentType("myjson");
            msg.setExpire(3000);
            conn.publish(msg, false);
            //Thread.sleep(10);
        }    
        System.out.println("publish");
        closed = true;
        for (int i=0; i<sleeptries; i++) {
            if (received >= count)
                break;
            Thread.sleep(100);
        }
        
        conn.disconnect();
        System.out.println("after disconnect");
    }
  
                    

    public int onMessage(IsmMqttMessage msg) {
        received++;
        System.out.println("Receive topic="+msg.getTopic()+" retain="+(msg.getRetain()?'1':'0')+" body=\""+msg.getPayload()+"\"");
        return 0;
    }


    public void onCompletion(IsmMqttResult result) {
    }


    public void onDisconnect(IsmMqttResult result) {
        System.out.println("" + result);
        Throwable e = result.getThrowable();
        if (e != null && !closed) {
            e.printStackTrace(System.out);
            System.exit(1);
        }
    }
}
