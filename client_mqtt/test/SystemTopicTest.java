import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;

/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
public class SystemTopicTest implements IsmMqttListener {
    static boolean closed = false;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.7", 16109, "/mqtt", "kwb");
        conn.setEncoding(IsmMqttConnection.BINARYWS4);
        conn.setListener(new SystemTopicTest());
        conn.setVerbose(true);
        conn.setClean(false);
        conn.connect();
        conn.subscribe("$SYS/me/Topic", 0);
        conn.subscribe("$SYSTEM", 1);
        conn.publish("$SYS/me/Topic", "junk");
        conn.publish("$SYSTEM", "junk");
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
