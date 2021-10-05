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

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;
import com.ibm.ism.mqtt.IsmMqttUserProperty;


public class SimpleTest implements IsmMqttListenerAsync {

	static boolean closed = false;
	
	public static void main(String [] args) throws Exception {
		IsmMqttConnection conn = new IsmMqttConnection("10.10.0.5", 16109, "/mqtt", "abc@defg");
	  	conn.setEncoding(IsmMqttConnection.WS5);
		conn.setAsyncListener(new SimpleTest());
		conn.setChunksize(1);
		conn.setUser("user", "password");
		conn.setVerbose(true);
		conn.setClean(false);
		conn.setTopicAlias(0, 0);    // 4,4
		conn.setRequestReason(1);
		conn.setRequestReplyInfo(true);
		conn.connect();
		System.out.println("connected");
		System.out.println("maxPacketSize="+conn.getServerMaxPacketSize());
		System.out.println("maxQoS="+conn.getMaxQoS());
		List<IsmMqttUserProperty> props = conn.getServerProperties();
		if (props != null) {
		    Iterator<IsmMqttUserProperty> it = props.iterator();
		    while (it.hasNext()) {
		        IsmMqttUserProperty prp = it.next();
		        System.out.println(""+prp.name()+" = "+prp.value());
		    }
		}
		//conn.unsubscribe("$SharedSubscription/");
		conn.subscribe("/fred/abc", 0, 89, true);
		System.out.println("subscribe1");
	    //conn.subscribe("/fred/abc", 0, 99, true);
	    //System.out.println("subscribe2");
		conn.subscribe("sa/#", 0, 88, true);
		conn.publish("/fred/abd", "This is a message");
		conn.setChunksize(-9999);
		IsmMqttMessage msg = new IsmMqttMessage("/fred/abc", "This is a message and more and more and more and more lasdjf asdlfkj jasldfj fjasdlf jfasdlf jfasdl jfalsd jfalsd jflasd jfalsd fjasdl fjadls jfalsd fjald jfsdl");
		msg.setContentType("text/fancy");
		msg.setExpire(1000);
		msg.addUserProperty("abc", "This is the property value");
		msg.addUserProperty("def", "This is another property value");
		msg.addUserProperty("long", "This is a very long property to make sure that setting the length more than 127 bytes works OK");
		msg.setQos(1);
		//msg.addUserProperty("def", "Another\ndef");
		
		conn.publish(msg, true);
		conn.setChunksize(5);

		Vector<Object>list = new Vector<Object>();
		list.add(3);
		list.add("ghi");
		IsmMqttMessage listmsg = new IsmMqttMessage("/fred/abc", list);
		listmsg.setCorrelation("3");
	  	listmsg.setReplyTo("temp/topic");
		conn.publish(listmsg);
		
		HashMap<String,Object>map = new HashMap<String,Object>();
		map.put("abc", 3);
		map.put("def", "ghi");
		IsmMqttMessage mapmsg = new IsmMqttMessage("sa/mual", map);
		conn.publish(mapmsg);
		
	  	conn.unsubscribe("fred");
		closed = true;
		conn.disconnect();
		System.out.println("disconnect");
	}

	public void onDisconnect(int rc, Throwable e) {
		System.out.println("Disconnect: rc="+rc);
		if (e != null && !closed) { 
			e.printStackTrace(System.out);
			System.exit(1);
		}			
			
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
