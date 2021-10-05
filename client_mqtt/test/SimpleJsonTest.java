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

import com.ibm.ism.jsonmsg.JsonConnection;
import com.ibm.ism.jsonmsg.JsonListener;
import com.ibm.ism.jsonmsg.JsonMessage;


public class SimpleJsonTest implements JsonListener {

    static boolean closed = false;
    
    public static void main(String [] args) throws Exception {
        JsonConnection conn = new JsonConnection("10.10.0.8", 16109, "/json-msg", "abc@def");
        conn.setEncoding(JsonConnection.JSON_TCP);
        conn.setListener(new SimpleJsonTest());
        // conn.setUser("user", "password");
        conn.setVerbose(true);
        conn.connect();
        //conn.subscribe("fred", false, "/fred/abc", false, 0, false);
        //conn.subscribe("wild", "sa/#", false);
        for (int i=0; i<1000; i++) {
            conn.publish("/fred/abc", "This is a message");
        }    
        closed = true;
        Thread.sleep(500);
        conn.closeConnection();
    }

    public void onDisconnect(int rc, Throwable e) {
        System.out.println("Disconnect: rc="+rc);
        if (e != null && !closed) { 
            e.printStackTrace(System.out);
            System.exit(1);
        }           
    }

    public void onMessage(JsonMessage msg) {
        System.out.println("Receive name=" + msg.getSubscription() + " topic="+msg.getTopic() + " body=\""+msg.getPayload()+"\"");
    }
    
}
