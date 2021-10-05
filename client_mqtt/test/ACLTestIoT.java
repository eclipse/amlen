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
import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;



public class ACLTestIoT implements IsmMqttListenerAsync {

    static boolean closed = false;
    static int  msgcnt = 0;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "g:x:abc:defg");
        IsmMqttConnection conn_any = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "g:x:typ:gateway");
        
        conn_any.setEncoding(IsmMqttConnection.TCP5);
        conn_any.setAsyncListener(new ACLTestIoT());
        conn_any.setUser("user", "password");
        conn_any.setVerbose(true);
        conn_any.setClean(true);
        conn_any.setSecure(0);
        conn_any.connect();
        conn_any.subscribe("iot-2/type/+/id/+/notify", 0);
        System.out.println("connected conn_any");
        
        conn.setEncoding(IsmMqttConnection.TCP5);
        conn.setAsyncListener(new ACLTestIoT());
        conn.setUser("user", "password");
        conn.setVerbose(true);
        conn.setClean(false);
        conn.setSecure(0);
        conn.connect();
        
        System.out.println("connected clean=0");

        String [] subs = new String[2];
        subs[0] = "iot-2/type/+/id/+/evt/+/fmt/json";
        subs[1] = "iot-2/type/dtype/id/devid0/evt/good/fmt/json";
        int [] qoss = new int[2];
        qoss[0] = 0;
        qoss[1] = 0;
        conn.subscribe(subs, qoss);
        conn.subscribe("iot-2/type/dtype/id/devid10/evt/bad/fmt/json", 0);
        conn.publish("iot-2/type/dtype/id/devid0/evt/good/fmt/json", "This is a good message 1");
        conn.publish("iot-2/type/dtype/id/devid10/evt/bad/fmt/json",  "This is a bad message 1");
        conn_any.publish("iot-2/type/dtype/id/devid10/evt/fred/fmt/json",  "This is a bad message 2");
        Thread.sleep(400);
        conn.disconnect();
        
        System.out.println("press any key");
        System.in.read();
        conn_any = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "g:x:typ:gateway");
        conn_any.setEncoding(IsmMqttConnection.TCP5);
        conn_any.setAsyncListener(new ACLTestIoT());
        conn_any.setUser("user", "password");
        conn_any.setVerbose(true);
        conn_any.setClean(true);
        conn_any.setSecure(0);
        conn_any.connect();
        conn_any.publish("iot-2/type/dtype/id/devid1/evt/good/fmt/json", "This is good message z2");
        conn_any.publish("iot-2/type/dtype/id/devid10/evt/fred/fmt/json",  "This is a bad message a3");
        Thread.sleep(200);
        conn = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "g:x:abc:defg");
        conn.setEncoding(IsmMqttConnection.TCP5);
        conn.setAsyncListener(new ACLTestIoT());
        conn.setUser("user", "password");
        conn.setVerbose(true);
        conn.setClean(false);
        conn.setSecure(0);
        conn.connect();
        Thread.sleep(500);
        conn.disconnect();
        
        conn.setClean(true);
        conn.connect();
        
        System.out.println("connected clean=1");
        Thread.sleep(4000);
        conn.subscribe("iot-2/type/abc/id/defg/notify", 0);
        conn.subscribe("iot-2/type/+/id/+/evt/+/fmt/json", 0);
        conn.subscribe("iot-2/type/dtype/id/devid0/evt/good/fmt/json", 0);
        conn.subscribe("iot-2/type/dtype/id/devid10/evt/bad/fmt/json", 0);
        conn.publish("iot-2/type/dtype/id/devid0/evt/good/fmt/json", "This is a good message 1");
        conn.publish("iot-2/type/dtype/id/devid10/evt/bad/fmt/json",  "This is a bad message 1");
        conn_any.publish("iot-2/type/dtype/id/devid10/evt/fred/fmt/json",  "This is a bad message 2");
        Thread.sleep(200);
        conn.disconnect();
        conn_any.disconnect();
        
        System.out.println("Message count = " + msgcnt);
        closed = true;
        System.exit(0);
    }

    public void onDisconnect(IsmMqttResult res) {
        int rc = res.getReasonCode();
        Throwable e = res.getThrowable();
        System.out.println("Disconnect: " + res);
        if (e != null && !closed) { 
            e.printStackTrace(System.out);
            System.exit(1);
        }           
            
    }
    
    public void onCompletion(IsmMqttResult res) {
        
    }

    public int onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
        msgcnt++;
        return 0;
    }
    
}

