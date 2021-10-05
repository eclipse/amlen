/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
import java.util.Vector;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;
import com.ibm.ism.mqtt.IsmMqttUserProperty;
import com.ibm.ism.mqtt.MqttRC;

/*
 * Test
 */
public class WillTest implements IsmMqttListenerAsync {
    static boolean proxy = false;
    static boolean verbose    = true; 
    static int encoding = IsmMqttConnection.WS5;
    
    static int failed = 0;
    static boolean closed = false;
    
    public static void main(String [] args) throws Exception {
        IsmMqttResult res;
        IsmMqttConnection conn;
        IsmMqttConnection connd;
        IsmMqttMessage will;
        String willTopic;
        String willTopicD;
        List<IsmMqttUserProperty> subProps;
        
        if (proxy) {
            conn = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "a:x:app");
            conn.setRequestReason(1);
            conn.setUser("user", "password");
            connd = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "d:x:typ:device");
            connd.setUser("user", "password");
            willTopic  = "iot-2/type/typ/id/device/evt/will/fmt/text";
            willTopicD = "iot-2/evt/will/fmt/text";
        } else {    
            conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@defg");
            conn.setRequestReason(9);
            connd = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@device");
            willTopic = "will/topic";
            willTopicD = willTopic;
        }    
        
        conn.setEncoding(encoding);
        conn.setAsyncListener(new WillTest());
        conn.setVerbose(true);
        conn.setClean(true);
        conn.setTopicAlias(4, 4);

        res = conn.connect(true);
        if (verbose)
            System.out.println("connected present=" + conn.isSessionPresent() + " " + res);
        if (res.getReasonCode() == 255) {
            System.err.println("FAILED: Unable to connect to "+conn.getAddress()+':'+conn.getPort());
            return;
        }
        subProps = (List<IsmMqttUserProperty>)new Vector<IsmMqttUserProperty>();
        subProps.add(new IsmMqttUserProperty("abc", "def"));    
        subProps.add(new IsmMqttUserProperty("keyword", "value1"));
        subProps.add(new IsmMqttUserProperty("keyword", "value2"));
        res = conn.subscribe(new String [] {willTopic}, new int [] {0}, 5, true, subProps);
        if (verbose)
            System.out.println("subscribe " + res);

        connd.setEncoding(encoding);
        connd.setAsyncListener(new WillTest());
        connd.setVerbose(true);
        connd.setClean(true);
        connd.setTopicAlias(4, 4);
        will = new IsmMqttMessage(willTopicD, "This is the will", 1, false);
        will.addUserProperty("keyword", "value1");
        will.addUserProperty("fred", "sam");
        will.addUserProperty("keyword", "value2");
        will.setContentType("MyContentType");
        will.setReplyTo("fred");
        will.setCorrelation("abc");
        connd.setWill(will,  100);
        res = connd.connect(true);
        if (verbose)
            System.out.println("connect with will present=" + conn.isSessionPresent() + " " + res);
        if (res.getReasonCode() == 255) {
            System.err.println("FAILED: Unable to connect to "+conn.getAddress()+':'+conn.getPort());
            return;
        }
        
        connd.disconnect(MqttRC.KeepWill, "Send Will");

        Thread.sleep(1000);
        
        res = conn.unsubscribe(new String [] {willTopic}, true, subProps);
        if (verbose)
            System.out.println("unsubscribe " + res);
        conn.disconnect();
        
        if (failed > 0) {
            System.err.println("CleanTest: failed="+failed);
            System.exit(1);;
        } 
        System.err.println("Test successful");
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

    /*
     * On completion
     * @see com.ibm.ism.mqtt.IsmMqttListenerAsync#onCompletion(com.ibm.ism.mqtt.IsmMqttResult)
     */
    public void onCompletion(IsmMqttResult result) {
        int rc = result.getReasonCode();
        System.out.println("Complete: " + result);
    }

    public void onDisconnect(IsmMqttResult result) {
        Throwable ex = result.getThrowable();
                
        System.out.println("" + result);
        if (ex != null && !closed) { 
            ex.printStackTrace(System.out);
            System.exit(1);
        }
        
    }
}
