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

import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import java.util.List;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;
import com.ibm.ism.mqtt.IsmMqttUserProperty;
import com.ibm.ism.mqtt.MqttRC;

/*
 * Test
 */
public class CleanTest implements IsmMqttListenerAsync {
    static boolean proxy = true;
    static boolean verbose    = true; 
    static int encoding = IsmMqttConnection.WS5;
    
    static int failed = 0;
    static boolean closed = false;
    
    public static void main(String [] args) throws Exception {
        IsmMqttResult res;
        IsmMqttConnection conn;
        IsmMqttConnection steal;
        IsmMqttConnection listen;
        String willtopic;
        
        if (proxy) {
            conn= new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "g:x:abc:defg");
            conn.setRequestReason(1);
            conn.setUser("user", "password");
            steal = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "g:x:abc:defg");
            steal.setRequestReason(1);
            steal.setUser("user", "password");
            listen = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "a:x:listen");
            listen.setUser("user", "password");
            willtopic = "iot-2/type/abc/id/defg/evt/will/fmt/text";
        } else {    
            conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@defg");
            conn.setRequestReason(9);
            steal = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@defg");
            listen = new IsmMqttConnection("10.10.0.5", 1883, "/mqtt", "a:x:listen");
            willtopic = "willtopic";
        }
        
        listen.setEncoding(encoding);
        listen.setAsyncListener(new CleanTest()); 
        listen.setClean(true);
        listen.connect();
        listen.subscribe(willtopic,  2, 99, true);
        
        conn.setEncoding(encoding);
        conn.setAsyncListener(new CleanTest());
        conn.setVerbose(true);
        conn.setClean(true);
        conn.setTopicAlias(4, 4);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connected clean: present=" + conn.isSessionPresent() + 
                " rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() == 255) {
            System.err.println("FAILED: Unable to connect to "+conn.getAddress()+':'+conn.getPort());
            return;
        }

        if (res.getReasonCode() != 0 || conn.isSessionPresent()) {
            System.err.println("FAILED: clean connection: rc=" + res.getReasonCode() + " present=" + conn.isSessionPresent());
            failed++;
        }
        IsmMqttResult resx = conn.unsubscribe("iot-2/type/+/id/+/evt/fred/fmt/sam", true);
        if (resx.getReasonCode() != 17) {
            System.out.println("FAILED unsubscribe is not 17 rc=" + resx.getReasonCode());
            failed++;
        }    
        conn.disconnect(4, null, 50, null);

        conn.setClean(false);
        conn.setExpire(0);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connected not clean with expire=0: present=" + conn.isSessionPresent() +
                " rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        conn.disconnect();

        conn.setClean(true);
        conn.setExpire(60);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connected clean with expire: present=" + conn.isSessionPresent() +
                 " rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 0 || conn.isSessionPresent()) {
            System.err.println("FAILED: clean with expire: rc=" + res.getReasonCode() + " present=" + conn.isSessionPresent());
            failed++;
        }
        
        IsmMqttMessage msg;
        byte [] badm = {(byte)'x', (byte)'y', (byte)'z', (byte)0xff};
        msg = new IsmMqttMessage("iot-2/type/dtype/id/devid0/evt/e/fmt/f", badm, 1, false);
        msg.setMessageType(IsmMqttMessage.TextMessage);
        res = conn.publish(msg, true);
        if (res.getReasonCode() != MqttRC.PayloadInvalid) {
            System.err.println("FAILED: bad rc on invalid payload: rc=" + res.getReasonCode());
            failed++;
        }
        msg = new IsmMqttMessage("iot-2/\u0001/id/i/evt/e/fmt/f", "mymessage", 1, false);
        res = conn.publish(msg, true);
        if (res.getReasonCode() != MqttRC.TopicInvalid) {
            System.err.println("FAILED: bad rc on invalid topic: rc=" + res.getReasonCode());
            failed++;
        }
        msg = new IsmMqttMessage("$sss", "mymessage", 1, false);
        res = conn.publish(msg, true);
        if (res.getReasonCode() != MqttRC.TopicInvalid) {
            System.err.println("FAILED: bad rc on invalid topic: rc=" + res.getReasonCode());
            failed++;
        }
        conn.disconnect();
        
        conn.setClean(false);
        conn.setExpire(60);
        conn.setWill(willtopic,  "This is a will message", 0, false, 30);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connected not clean with expire: present=" + conn.isSessionPresent() +
                " rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 0 || !conn.isSessionPresent()) {
            System.err.println("FAILED: not clean with expire connection: rc=" + res.getReasonCode() + " present=" + conn.isSessionPresent());
            failed++;
        }
        
        steal.setClean(false);
        steal.setExpire(0);
        res = steal.connect(true);
        System.out.println("Reconnect with will delay: " + res);
        steal.disconnect();
        
        conn.setWill(null,  null, 0, false);
        conn.setClientID("A:x:myapp:inst");
        conn.setClean(true);
        conn.setExpire(60);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connected app not clean with expire: " + res);
        conn.disconnect();
        
        conn.setClientID("A:x:myapp:inst");
        conn.setClean(false);
        conn.setExpire(60);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connect 0 rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 0) {
            System.err.println("FAILED: clientID with 0 byte: rc=" + res.getReasonCode());
            failed++;
        }
        if (res.getReasonCode() != 0 || !conn.isSessionPresent()) {
            System.err.println("FAILED: not clean app with expire connection: " + res);
            failed++;
        } else {
            res = conn.subscribe("iot-2/type/abc/id/defg/evt/+/fmt/+", 0, 2, true);
            System.out.println("subscribe1 " + res);
        }
        conn.disconnect();
        
        conn.setClean(false);
        conn.setExpire(60);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connect 0 rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 0) {
            System.err.println("FAILED: not clean app: rc=" + res.getReasonCode());
            failed++;
        }
        if (res.getReasonCode() != 0 || !conn.isSessionPresent()) {
            System.err.println("FAILED: not clean app with expire connection: " + res);
            failed++;
        } else {
            res = conn.subscribe("iot-2/type/abc/id/defg/evt/+/fmt/+", 0, 3, true);
            System.out.println("subscribe2 " + res);
            conn.publish("iot-2/type/abc/id/defg/evt/msg/fmt/text", "This is a test");
            Thread.sleep(100);
        }
        conn.disconnect();
        
        conn.setClientID("g:x:abc\uffff:defg");
        conn.setEncoding(encoding);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connect nonchar rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 133) {
            System.err.println("FAILED: clientID with nonchar: rc=" + res.getReasonCode());
            failed++;
        }
        
        conn.setClientID("g:x:abc\n:defg");
        conn.setEncoding(encoding);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connect c0 rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        int expect = 133;
        if (res.getReasonCode() != expect) {
            System.err.println("FAILED: clientID with CC: rc=" + res.getReasonCode());
            failed++;
        }
        
        conn.setClientID(proxy ? "fred" : "__abc@defg");
        conn.setEncoding(encoding);
        res = conn.connect(true, false);
        if (verbose)
            System.out.println("connect __ rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        expect = proxy ? 135 : 133;
        if (res.getReasonCode() != expect) {
            System.err.println("FAILED: clientID with leading __: rc=" + res.getReasonCode());
            failed++;
        }
        
        conn.setClientID("g:x:abc:defg");
        conn.setUser("fr\ned", "sam");
        conn.setEncoding(encoding);
        res = conn.connect(true);
        if (verbose)
            System.out.println("connect user rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 130) {
            System.err.println("FAILED: clientID with bad username: rc=" + res.getReasonCode());
            failed++;
        }
        
        conn.setClean(true);
        if (proxy) {
            conn.setUser("user", "password");
        } else {    
            conn.setUser(null,  null);
        } 
        res = conn.connect(true);
        if (verbose)
            System.out.println("connected clean present=" + conn.isSessionPresent() +
                 " rc="+res.getReasonCode()+" reason="+res.getReason()+" except="+res.getThrowable());
        if (res.getReasonCode() != 0) {
            System.err.println("FAILED: clean with expire: rc=" + res.getReasonCode() + " present=" + conn.isSessionPresent());
            failed++;
        } else {
            int [] qoss = new int [2];
            String [] topics = new String [2];
            qoss[0] = 0;
            qoss[1] = 1;
            topics[0] = "bad\ntopic";
            topics[1] = proxy ? "bad" : "bad\nbad";
            res = conn.subscribe(topics, qoss, 0, true);
            System.out.println(""+res);
            
            topics[1] = "iot-2/type/dtype/id/devid0/evt/event/fmt/json";
            res = conn.subscribe(topics, qoss, 0, true);
            System.out.println(""+res);
        }
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
