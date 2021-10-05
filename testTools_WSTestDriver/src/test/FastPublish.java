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
import org.eclipse.paho.mqttv5.client.MqttAsyncClient;
import org.eclipse.paho.mqttv5.client.MqttCallback;
import org.eclipse.paho.mqttv5.client.MqttConnectionOptions;
import org.eclipse.paho.mqttv5.client.MqttDisconnectResponse;
import org.eclipse.paho.mqttv5.common.MqttException;
import org.eclipse.paho.mqttv5.common.MqttMessage;
import org.eclipse.paho.mqttv5.common.MqttSubscription;
import org.eclipse.paho.mqttv5.common.packet.MqttProperties;


public class FastPublish implements MqttCallback{
    static int count = 200;
    static int sleepus = 0;
    static boolean verbose = false;

    static MqttAsyncClient con;
    static MqttConnectionOptions opt;
    static MqttProperties prp;
    volatile int    rcvcount;
    static FastPublish fp;

    public static void main(String [] args) throws Exception {
        int  i;
        if (args.length > 0) {
            count = Integer.parseInt(args[0]);
            if (args.length > 1) {
                sleepus = Integer.parseInt(args[1]);
            }
        }

        try {
            opt = new MqttConnectionOptions();
            con = new MqttAsyncClient("tcp://10.10.0.5:16109", "a:x:1234");
            fp = new FastPublish();
            con.setCallback(fp);
            opt.setUserName("kwb");
            opt.setPassword("password".getBytes());
            opt.setReceiveMaximum(20000);

            opt.setTopicAliasMaximum(0);

            IMqttToken connToken = con.connect(opt);
            connToken.waitForCompletion();
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

            for (i=0; i<count; i++) {
                MqttMessage msg = new MqttMessage();
                msg.setQos(0);
                String payload = "This is message: " + i;
                msg.setPayload(payload.getBytes());
                if (verbose)
                    System.out.println("Publish: " + payload);
                con.publish("iot-2/type/mytype/id/myid/evt/myevt/fmt/text", msg);
                Thread.sleep(sleepus/1000, (sleepus*1000)%1000000);
            }
            System.out.println("Publish complete.  Count=" + i);

            Thread.sleep(500);
            System.out.println("Disconnect: count=" + fp.getCount());
            con.disconnect(0, null, null, 4, null);
        } catch (Exception e)  {
            e.printStackTrace();
        } finally {
            con.close(true);
            System.out.println("connection.close(true)");
        }
        System.exit(0);
    }

    public int getCount() {
        return rcvcount;
    }

    public void disconnected(MqttDisconnectResponse response) {
        System.out.println("Disconnect: count=" + rcvcount + " " + response);
    }

    public void mqttErrorOccurred(MqttException ex) {
        ex.printStackTrace();
    }

    public void messageArrived(String topic, MqttMessage message) throws Exception {
        rcvcount++;
        if (verbose)
            System.out.println("Receive: " + message);
    }

    public void deliveryComplete(IMqttToken token) {
        // System.out.println("Complete: " + token);
    }

    public void connectComplete(boolean reconnect, String serverURI) {
        System.out.println("Connect complete: " + serverURI);
    }

    public void authPacketArrived(int reasonCode, MqttProperties properties) {
    }
}

