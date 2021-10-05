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



public class ACLTest implements IsmMqttListener {

    static boolean closed = false;
    static int  msgcnt = 0;
    
    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "g:abc:defg");
        conn.setEncoding(IsmMqttConnection.PROXY5);
        conn.setListener(new ACLTest());
        conn.setUser("user", "password");
        conn.setVerbose(true);
        conn.setClean(false);
        conn.connect();
        
        System.out.println("connected clean=0");
        conn.subscribeSelector("fred/+/+", "JMS_Topic aclcheck('testacl',1,2) is not false", 0);
        conn.publish("fred/abc/good", "This is a good message 1");
        conn.publish("fred/abc/bad",  "This is a bad message 1");
        conn.subscribeSelector("$share/sname/shared/+/+", "JMS_Topic aclcheck('testacl',1,2) is not false", 0);
        conn.publish("shared/abc/good", "This is a good message 2");
        conn.publish("shared/abc/bad",  "This is a bad message 2");
        Thread.sleep(200);
        conn.disconnect();
        
        conn.connect();
        System.out.println("connected clean=0");
        conn.publish("fred/abc/good", "This is a good message 1");
        conn.publish("fred/abc/bad",  "This is a bad message 1");
        conn.publish("shared/abc/good", "This is a good message 2");
        conn.publish("shared/abc/bad",  "This is a bad message 2");
        Thread.sleep(200);
        conn.disconnect();
        
        conn.setClean(true);
        conn.connect();
        System.out.println("connected clean=1");
        conn.subscribeSelector("fred/+/+", "JMS_Topic aclcheck('testacl',1,2) is not false", 0);
        conn.publish("fred/abc/good", "This is a good message 3");
        conn.publish("fred/abc/bad",  "This is a bad message 3");
        conn.subscribeSelector("$share/sname/shared/+/+", "JMS_Topic aclcheck('testacl',1,2) is not false", 0);
        conn.publish("shared/abc/good", "This is a good message 4");
        conn.publish("shared/abc/bad",  "This is a bad message 4");
        Thread.sleep(200);
        conn.disconnect();
        
        System.out.println("Message count = " + msgcnt);
        closed = true;
    }

    public void onDisconnect(int rc, Throwable e) {
        if (e != null && !closed) { 
            e.printStackTrace(System.out);
            System.exit(1);
        }           
            
    }

    public void onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
        msgcnt++;
    }
    
}
