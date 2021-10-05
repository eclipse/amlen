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
import com.ibm.ism.mqtt.IsmMqttResult;


public class IoTTest implements IsmMqttListener {

    static volatile boolean closed = false;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("kwb5.example.com", 1883, "/mqtt", "d:z:typ:device");
        IsmMqttConnection conn2 = new IsmMqttConnection("kwb5", 8883, "/mqtt", "a:x:app");
        IsmMqttConnection conn2a = new IsmMqttConnection("kwb5", 8883, "/mqtt", "a:x:app");
        IsmMqttConnection conn3 = new IsmMqttConnection("kwb5", 1883, "/mqtt", "d:quickstart:typ2:device");
        IsmMqttConnection conn4 = new IsmMqttConnection("kwb5", 1883, "/mqtt", "g:x:abc:defg");
        IsmMqttConnection conn5 = new IsmMqttConnection("kwb5", 16104, "/mqtt", "billing");
        IsmMqttConnection conn6 = new IsmMqttConnection("kwb5", 1883, "/mqtt", "a:x:app1");
        
        conn5.setEncoding(IsmMqttConnection.WS5);
        conn5.setListener(new IoTTest());
        conn5.setVerbose(true);
        conn5.setSecure(30);
        System.out.println("conn5="+conn);
        conn5.connect();
        conn5.subscribe("iot-2/+/type/+/id/+/mon", 0);
        conn5.subscribe("iot-2/+/app/+/mon", 0);
        // conn5.publish("iot-2/x/\5/abc", "This is bad");
        
        conn.setEncoding(IsmMqttConnection.WS3);
        conn.setListener(new IoTTest());
        conn.setUser("kwb", "password");
        conn.setVerbose(true);
        conn.setSecure(0);
        conn.setHost("kwb.1234");
        conn.setMaxReceive(512);
        long start = System.currentTimeMillis();
        System.out.println("conn="+conn);
        try {
            conn.connect();
        } catch (Exception e) {    
            System.out.println("Connection failed: " + e + "time="+(System.currentTimeMillis()-start));
        }
        System.out.println("connect");
        conn2.setEncoding(IsmMqttConnection.WS5);
        conn2.setListener(new IoTTest());
        conn2.setUser("kwb", "password");
        conn2.setVerbose(true);
        conn2.setSecure(3);
        conn2.connect();
        System.out.println("connect2");
        conn2.subscribe("iot-2/type/typ/id/device/evt/+/fmt/json", 2);
        System.out.println("subscribe");
        
        // conn.subscribe("iot-2/cmd/#", 0);
        
        conn.publish("iot-2/evt/fred/fmt/json", "This is a message");
        conn.publish("iot-2/evt/fred2/fmt/json", "This is a message2");
        conn.publish("iot-2/evt/fred3/fmt/json", "This is a message and more and more and more and more lasdjf asdlfkj jasldfj fjasdlf jfasdlf jfasdl jfalsd jfalsd jflasd jfalsd fjasdl fjadls jfalsd fjald jfsdl");
        System.out.println("publish");
        
        conn2.publish("iot-2/type/typ/id/device/evt/fred/fmt/json", "This is a message 1", 0, true);
        //conn2.publish("iot-2/type/typ/id/device/evt/fred/fmt/xml", "This is a message x2");
        //conn2.publish("iot-2/type/typ/id/device/cmd/fred", "This is a message x3");
        conn3.setEncoding(IsmMqttConnection.WS5);
        conn3.setListener(new IoTTest());
        conn3.setVerbose(true);
        
        System.out.println("connect 3");
        conn3.connect();
        conn3.publish("iot-2/evt/fred/fmt/json", "This is a message 4");
        
        closed = true;

        conn4.setEncoding(IsmMqttConnection.TCP3);
        conn4.setListener(new IoTTest());
        conn4.setVerbose(true);
        conn4.setUser("kwb", "password");
        System.out.println("connect 4");
        conn4.connect();
        String [] sublist = { "#", "$" };  
        int [] qoslist = { 1, 1};
        IsmMqttResult res = conn4.subscribe(sublist, qoslist, 0, true);
        System.out.println("subscribe bad rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        int rqos = conn4.subscribe("#", 0);
        System.out.println("Returned QoS=" + rqos);
        conn4.publish("iot-2/type/typ/id/device/evt/fred/fmt/json", "This is a message 41", 0, true);
        conn4.publish("iot-2/type/typ/id/device/evt/fred/fmt/json", "This is a message 42", 0, true);
        conn4.unsubscribe("iot-2/type/type/id/mydev/cmd/+/fmt/+");
        
        closed = true;
        System.out.println("connect 6");
        try {
            conn6.setEncoding(IsmMqttConnection.WS5);
            conn6.setListener(new IoTTest());
            conn6.setVerbose(true);
  
            Thread.sleep(300);
            conn6.connect();
        } catch (Exception e) {
            System.out.println("Exception: " + e);
        }
        
        System.out.println("try reuse");
        conn2a.setEncoding(IsmMqttConnection.WS5);
        conn2a.setListener(new IoTTest());
        conn2a.setUser("kwb", "password");
        conn2a.setVerbose(true);
        conn2a.setSecure(3);
        conn2a.connect();
        
        System.out.println("after connect");
        closed = true;
        Thread.sleep(5000);
        System.out.println("after sleep");
        
        conn.disconnect();
        conn2a.disconnect();
        conn3.disconnect();
        conn4.disconnect();
        Thread.sleep(300);
        conn5.disconnect();
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
