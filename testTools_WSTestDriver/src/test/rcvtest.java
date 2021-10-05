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
package test;

import org.eclipse.paho.mqttv5.client.IMqttToken;
import org.eclipse.paho.mqttv5.client.IMqttToken;
import org.eclipse.paho.mqttv5.client.MqttAsyncClient;
import org.eclipse.paho.mqttv5.client.MqttCallback;
import org.eclipse.paho.mqttv5.client.MqttConnectionOptions;
import org.eclipse.paho.mqttv5.client.MqttDisconnectResponse;
import org.eclipse.paho.mqttv5.common.MqttException;
import org.eclipse.paho.mqttv5.common.MqttMessage;
import org.eclipse.paho.mqttv5.common.MqttSubscription;
import org.eclipse.paho.mqttv5.common.packet.MqttConnAck;
import org.eclipse.paho.mqttv5.common.packet.MqttProperties;


public class rcvtest implements MqttCallback{
    static MqttAsyncClient con;
    static MqttConnectionOptions opt;
    static MqttProperties prp;
    
    public static void main(String [] args) throws Exception {
        try {
            opt = new MqttConnectionOptions();
            con = new MqttAsyncClient("tcp://10.10.0.5:16109", "a:x:1234");
            con.setCallback(new rcvtest());
            opt.setUserName("kwb");
            opt.setPassword("password".getBytes());
            
            opt.setTopicAliasMaximum(0);
            
            IMqttToken connToken = con.connect(opt);
            connToken.waitForCompletion();
            MqttConnAck connack = (MqttConnAck)connToken.getResponse();
            if (connack != null) {
                MqttProperties props = connack.getProperties();
                if (props != null) {
                    System.out.println("wildcard=" + props.isWildcardSubscriptionsAvailable());
                }
            }
            System.out.println("connect: " + connToken);
            
            MqttSubscription [] subs = new MqttSubscription[1];
            subs[0] = new MqttSubscription("iot-2/type/mytype/id/myid/evt/+/fmt/+", 2);
            IMqttToken subToken = null;
            try {
                subToken = con.subscribe(subs);
                subToken.waitForCompletion();
                System.out.println("Subscribe: " + subToken);
                int [] rc =  subToken.getGrantedQos();
                System.out.println("Subscribe rc=" + ((rc != null && rc.length > 0) ? rc[0] : "null"));
            } catch (Exception e) {
                e.printStackTrace();
            }
            
            MqttMessage msg = new MqttMessage();
            msg.setQos(0);
            msg.setPayload("This is a message".getBytes());
            con.publish("iot-2/type/mytype/id/myid/evt/myevt/fmt/text0", msg);
            System.out.println("Publish");

            /* Send QoS=1 to a topic without a subscription.  Should return 0x10 */
            MqttMessage msg1 = new MqttMessage();
            msg1.setQos(1);
            msg1.setPayload("This is a message".getBytes());
            IMqttToken pub1 = con.publish("iot-2/type/mytype/id/myid/evt/myevt/fmt/text1", msg1);
            pub1.waitForCompletion();
            int [] rc = pub1.getReasonCodes();
            System.out.println("Publish1 rc=" + ((rc != null && rc.length > 0) ? rc[0] : "Not Set") + ' ');
            
            /* Send QoS=2 to a topic without a subscription.  Should return 0x10 */
            MqttMessage msg2 = new MqttMessage();
            msg2.setQos(2);
            msg2.setPayload("This is a message".getBytes());
            IMqttToken pub2 = con.publish("iot-2/type/mytype/id/myid/evt/myevt/fmt/text2", msg2);
            pub2.waitForCompletion();
            rc = pub2.getReasonCodes();
            System.out.println("Publish2 rc=" + ((rc != null && rc.length > 0) ? rc[0] : "Not Set"));
            
            Thread.sleep(500);
            System.out.println("Disconnect");
            con.disconnect(0, null, null, 4, null);
        } catch (Exception e)  {
            e.printStackTrace();
        } finally {    
            con.close(true);
            System.out.println("connection.close(true)");
        }   
        System.exit(0);
    }

    public void disconnected(MqttDisconnectResponse response) {
        System.out.println("Disconnect: "+response);
    }

    public void mqttErrorOccurred(MqttException ex) {
        ex.printStackTrace();
    }

    public void messageArrived(String topic, MqttMessage message) throws Exception {
        System.out.println("Receive: topic=" + topic + " message=" + message);
    }

    public void deliveryComplete(IMqttToken token) {
        System.out.println("Complete: " + token);
    }

    public void connectComplete(boolean reconnect, String serverURI) {
        System.out.println("Connect complete: " + serverURI);
    }

    public void authPacketArrived(int reasonCode, MqttProperties properties) {
    }
}
