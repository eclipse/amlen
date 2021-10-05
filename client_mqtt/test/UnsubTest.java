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
import java.util.Iterator;
import java.util.List;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;
import com.ibm.ism.mqtt.IsmMqttUserProperty;

public class UnsubTest implements IsmMqttListenerAsync {
    static boolean closed = false;
    static boolean verbose = false; 
    static int failed = 0;
    final static int count = 100;
    
    public static void main(String [] args) throws Exception {
        IsmMqttResult res;
        IsmMqttConnection conn;
        String topics [] = new String [count]; 
        int qoss [] = new int [count];
        int  i;
        
        conn= new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@defg");
        conn.setRequestReason(9);
        conn.setEncoding(IsmMqttConnection.TCP4);
        conn.setAsyncListener(new UnsubTest());
        conn.setVerbose(true);
        conn.setClean(false);
        conn.setTopicAlias(4, 4);
        res = conn.connect(true);
        
        for (i=0; i<count; i++) {
            topics[i] = "/CTT/007/SubTopic/" + i;
            qoss[i] = 2;
        }
        res = conn.subscribe(topics,  qoss, 12, true);
        showCompletion(res);
        conn.disconnect();
        
        conn= new IsmMqttConnection("10.10.0.5", 16309, "/mqtt", "abc@defg");
        conn.setRequestReason(9);
        conn.setEncoding(IsmMqttConnection.TCP4);
        conn.setAsyncListener(new UnsubTest());
        conn.setVerbose(true);
        conn.setClean(false);
        conn.setTopicAlias(4, 4);
        res = conn.connect(true);
        showCompletion(res);
        
        res = conn.unsubscribe(topics, true);
        showCompletion(res);
        conn.disconnect();
    }    
    

    /*
     * When a message is received
     * @see com.ibm.ism.mqtt.IsmMqttListenerAsync#onMessage(com.ibm.ism.mqtt.IsmMqttMessage)
     */
    public int onMessage(IsmMqttMessage msg) {
        System.out.println("Receive topic="+msg.getTopic()+" body=\""+msg.getPayload()+"\"");
        if (msg.getSubscriptionID() > 0)
            System.out.println("   SubscriptionID = " + msg.getSubscriptionID());
        if (msg.getReplyTo() != null)
            System.out.println("   ReplyTopic = " + msg.getReplyTo());
        if (msg.getCorrelation() != null)
            System.out.println("   Correlation = "+msg.getCorrelationString());
        if (msg.getContentType() != null) 
            System.out.println("   ContentType = "+msg.getContentType());
        List<IsmMqttUserProperty> props = msg.getUserProperties();
        if (props != null) {
            Iterator<IsmMqttUserProperty> it = props.iterator();
            while (it.hasNext()) {
                IsmMqttUserProperty prp = it.next();
                System.out.println("   "+prp.name()+" = "+prp.value());
            }
        }
        return 0;
    }
    
    public static void showCompletion(IsmMqttResult result) {
        String op = result.getOperationName();
        int rc = result.getReasonCode();
        String reason = result.getReason();
        System.out.println("Complete: op="+op+" rc="+rc+(reason != null ? (" reason="+reason): ""));
    }

    /*
     * On completion
     * @see com.ibm.ism.mqtt.IsmMqttListenerAsync#onCompletion(com.ibm.ism.mqtt.IsmMqttResult)
     */
    public void onCompletion(IsmMqttResult result) {
        String op = result.getOperationName();
        int rc = result.getReasonCode();
        String reason = result.getReason();
        System.out.println("Complete: op="+op+" rc="+rc+(reason != null ? (" reason="+reason): ""));
    }

    public void onDisconnect(IsmMqttResult result) {
        int rc = result.getReasonCode();
        Throwable ex = result.getThrowable();
        String reason = result.getReason();
                
        System.out.println("Disconnect: rc="+rc+ (reason != null ? (" reason="+reason): ""));
        if (ex != null && !closed) { 
            ex.printStackTrace(System.out);
            System.exit(1);
        }
        
    }
}
