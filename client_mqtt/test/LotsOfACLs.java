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

import java.util.Random;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;


public class LotsOfACLs implements IsmMqttListener {

    static boolean closed = false;
    static int  count = 200000;
    static int  percent = 10;
    static volatile int  msgcnt = 0;
    static volatile int  done = 0;
    static long start_time;
    static long end_time;
    static int  expected;
    
    public static void main(String [] args) throws Exception {
        int i;
        Random rnd = new Random(System.currentTimeMillis());
        int range = 1000/percent;
        
        IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "g:abc:defg");
        conn.setEncoding(IsmMqttConnection.PROXY4);
        conn.setListener(new LotsOfACLs());
        conn.setUser("user", "password");
        conn.setVerbose(true);
        conn.setClean(true);
        conn.connect();

        System.out.println("connected clean=1");
        conn.subscribeSelector("fred/+/+", "JMS_Topic aclcheck('testacl',1,2) is not false", 0);
        start_time = System.currentTimeMillis();
        end_time = 0;
        
        for (i=0; i<count; i++) {
            int which = rnd.nextInt(range);
            conn.publish("fred/dtype/devid"+which, "This is a message"+expected);
            if (which <= 9)
                expected++;
        }
        done = expected;
        for (i=0; i<100; i++) {
            Thread.sleep(100);
            if (end_time != 0)
                break;
        }
        System.out.println("Selector Count="+count+" Expected="+expected+" Received="+msgcnt+" Time="+(end_time-start_time)+"ms");
        
        conn.subscribe("bill/+/+", 0);
        start_time = System.currentTimeMillis();
        end_time = 0;
        msgcnt = 0;
        done = 0;
        expected = 0;
        
        for (i=0; i<count; i++) {
            int which = rnd.nextInt(range);
            if (which <= 9) {
                conn.publish("bill/dtype/devid0", "This is a message "+expected);
                expected++;
            } else {
                conn.publish("dave/dtype/devid00", "This is a message");
            }
        }
        done = expected;
        for (i=0; i<100; i++) {
            Thread.sleep(100);
            if (end_time != 0)
                break;
        }
        System.out.println("Normal   Count="+count+" Expected="+expected+" Received="+msgcnt+" Time="+(end_time-start_time)+"ms");
        
        closed = true;
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
        //System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
        msgcnt++;
        int check = done;
        if (check != 0 && msgcnt == check) {
            end_time = System.currentTimeMillis();
        }
            
    }
}
