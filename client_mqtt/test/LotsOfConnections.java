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


public class LotsOfConnections implements IsmMqttListener {

    static boolean closed = false;
    String cid;
    
    public static void main(String [] args) throws Exception {
        int i;
        IsmMqttConnection conn5 = new IsmMqttConnection("10.10.0.5", 16104, "/mqtt", "billing");
        conn5.setEncoding(IsmMqttConnection.TCP5);
        conn5.setListener(new IoTTest());
        conn5.setVerbose(true);
        conn5.setSecure(3);
        conn5.connect();
        conn5.subscribe("iot-2/+/type/+/id/+/mon", 0);
        conn5.subscribe("iot-2/+/app/+/mon", 0);
        IsmMqttConnection lastConn = null;
        
        closed = true;
        for (i=0; i<100; i++) {
            String clientid = "a:x:" + i;
            IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 8883, "/mqtt", clientid);
            conn.setEncoding(IsmMqttConnection.TCP5);
            conn.setSecure(3);
            conn.setListener(new LotsOfConnections(clientid));
            conn.setVerbose(true);
            conn.setUser("kwb", "password");
            System.out.println("Connect "+clientid+" i="+i);
            conn.connect();
            lastConn = conn;
            // conn.disconnect();
        }    
        lastConn.disconnect();
        conn5.disconnect();
    }
    
    public LotsOfConnections(String cid) {
        this.cid = cid;
    }

    public void onDisconnect(int rc, Throwable e) {
        System.out.println("Disconnect " + cid + ": rc="+rc);
        if (e != null && !closed) 
            e.printStackTrace(System.out);
            
    }

    public void onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
    }
    
}
