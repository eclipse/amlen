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
package svt.scale.vehicle;

import java.util.HashSet;
import java.util.Hashtable;
import java.util.Set;

import svt.util.SVTLog;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVTLog.TTYPE;
import svt.vehicle.SVTVehicle_MQTT;

import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class SVTVehicleScale_MQTTThread extends SVTVehicleScale implements Runnable {
    static TTYPE TYPE = TTYPE.MQTT;
    String mode = "connect_once";
    String ismserver = "";
    Integer ismport = null;
    String topicName = "";
    String appId = "";
    long minutes = 0;
    long messages = 0;
    long max = 0;
    SVTStats stats = null;
    Hashtable<MqttTopic, MqttClient> tlist;
    // Vector<Thread> list = null;
    // Vector<SVTStats> statsList = null;
    long carId = 0;
    boolean order = true;
    Integer qos = 0;
    Set<MqttTopic> failures = new HashSet<MqttTopic>();
    boolean listener = false;
    boolean ssl = false;
    String userName = null;
    String password = null;
    boolean genClientId = true;

    SVTVehicleScale_MQTTThread(String mode, String ismserver, Integer ismport, String appId, Long minutes, Long hours,
                    Long messages, SVTStats stats, long carId, boolean order, Integer qos, boolean listener,
                    boolean ssl, String userName, String password, boolean genClientId) {
        this.mode = mode;
        this.ismserver = ismserver;
        this.ismport = ismport;
        this.appId = appId;
        if (minutes != null) {
            this.minutes = minutes;
        }
        if (hours != null) {
            this.minutes += (hours * 60);
        }
        if (messages != null) {
            this.messages = messages;
        }
        this.max = this.minutes * 60000;
        this.stats = stats;
        this.carId = carId;
        this.order = order;
        this.qos = qos;
        this.listener = listener;
        this.ssl = ssl;
        this.userName = userName;
        this.password = password;
        this.genClientId = genClientId;
    }

    public void run() {
        String pClientId = null;

		if (genClientId) {
			pClientId = SVTUtil.MyClientId(TYPE);
		} else {
			pClientId = userName;
		}
		
        this.topicName = SVTUtil.MyTopicId(appId, pClientId);
        String message = "";
        // SVTLog.log(TYPE, "thread created");

        SVTVehicle_MQTT vehicle = new SVTVehicle_MQTT(ismserver, ismport, carId, qos, ssl, userName, password);
        if (this.listener == true)
            vehicle.startListener();

        try {
            this.stats.reinit();

            SVTLog.log3(TYPE, pClientId + " to send messages on topic " + topicName + " Connection mode is " + mode);

            if ("connect_each".equals(mode)) {
                while (true) {
                    this.stats.beforeMessage();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    int x = (int) (Math.random() * 1000.0);
                    int y = (int) (Math.random() * 1000.0);
                    vehicle.sendMessage(topicName, pClientId, "GPS " + x + "," + y, this.stats.increment());
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    this.stats.afterMessageRateControl();
                }
            } else if ("connect_once".equals(mode)) {
                MqttClient client = vehicle.getClient(pClientId);
                MqttTopic topic = client.getTopic(topicName);
                while (true) {
                    this.stats.beforeMessage();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    int x = (int) (Math.random() * 1000.0);
                    int y = (int) (Math.random() * 1000.0);
                    vehicle.sendMessage(topic, "GPS " + x + "," + y);
                    this.stats.increment();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    this.stats.afterMessageRateControl();
                }
                try {
                    client.disconnect();
                } catch (MqttException e) {
                    SVTLog.logex(TYPE, pClientId + ": Exception caught from client.disconnect()", e);
                }
            } else if ("batch".equals(mode)) {

                MqttClient client = vehicle.getClient(pClientId);
                MqttTopic topic = client.getTopic(topicName);
                boolean sent = false;
                while (true) {
                    this.stats.beforeMessage();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    sent = false;
                    int x = (int) (Math.random() * 1000.0);
                    int y = (int) (Math.random() * 1000.0);
                    while (sent == false) {
                        if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                        || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                            break;
                        }
                        try {
                            if (topic == null) {
                                client = vehicle.getClient(pClientId);
                                topic = client.getTopic(topicName);
                            }
                            vehicle.sendMessage(topic, "GPS " + x + "," + y);
                            sent = true;
                        } catch (MqttException e) {
                            topic = null;
                            client = null;
                        }
                    }
                    this.stats.increment();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    this.stats.afterMessageRateControl();
                }
                try {
                    client.disconnect();
                } catch (MqttException e) {
                    SVTLog.logex(TYPE, pClientId + ": Exception caught from client.disconnect()", e);
                }
            } else if ("batch_with_order".equals(mode)) {
                MqttClient myclient = null;
                MqttTopic mytopic = null;
                int tlistsize = 50;
                tlist = new Hashtable<MqttTopic, MqttClient>();
                this.stats.setTopicCount(tlistsize);
                for (i = 0; i < tlistsize; i++) {
                    myclient = vehicle.getClient(pClientId + "_" + i);
                    mytopic = myclient.getTopic(topicName + "_" + i);
                    tlist.put(mytopic, myclient);
                }

                int x = (int) (Math.random() * 1000.0);
                int y = (int) (Math.random() * 1000.0);
                int i = 0;
                MqttMessage textMessage = null;
                if (this.order == false) {
                    String text = "GPS " + x + "," + y;
                    textMessage = new MqttMessage(text.getBytes());
                    textMessage.setQos(this.qos);
                }

                Set<MqttTopic> topicSet = tlist.keySet();

                while (true) {
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    this.stats.beforeMessage();
                    for (MqttTopic topic : topicSet) {
                        if (this.order == true) {
                            String text = "ORDER:" + tlist.get(topic).getClientId() + ":" + i + ":" + "GPS " + x + ","
                                            + y;
                            textMessage = new MqttMessage(text.getBytes());
                            textMessage.setQos(this.qos);
                        }
                        try {
                            topic.publish(textMessage);
                            this.stats.increment();
                        } catch (MqttException e) {
                            failures.add(topic);
                        }
                    }

                    synchronized (failures) {
                        failures.notifyAll();
                    }
                    this.stats.afterMessageRateControl();
                    i++;
                }
                this.stats.setTopicCount(0);
                tlist.clear();
            }

        } catch (Throwable e) {
            SVTLog.logex(TYPE, "sendMessage", e);
            SVTUtil.stop(true);
        }

        if (this.listener == true)
            vehicle.stopListener();

        int t = SVTUtil.threadcount(-1);

        String text = "mqttclient ," + pClientId + ", finished__" + message + ". Ran for ,"
                        + this.stats.getRunTimeSec() + ", sec. Sent ," + this.stats.getCount() + ", messages. Rate: ,"
                        + this.stats.getRatePerMin() + ", msgs/min ," + this.stats.getRatePerSec() + ", msgs/sec " + t
                        + " clients remain.";

        SVTLog.log3(TYPE, text);
        return;
    }

    class failuresThread implements Runnable {
        MqttClient myclient = null;
        Set<MqttTopic> topics = new HashSet<MqttTopic>();
        Set<MqttTopic> connected = new HashSet<MqttTopic>();
        SVTStats stats = null;

        failuresThread(SVTStats stats) {
            this.stats = stats;
        }

        public void run() {
            long failedCount = 0;
            while (this.stats.getDeltaRunTimeMS() < max) {
                failedCount = 0;
                synchronized (failures) {
                    try {
                        failures.wait();
                        topics.addAll(failures);
                        failures.clear();
                    } catch (InterruptedException e) {
                    }
                }
                connected.clear();
                for (MqttTopic topic : topics) {
                    myclient = tlist.get(topic);
                    if (myclient.isConnected() == true) {
                        connected.add(topic);
                    } else {
                        for (int j = 0; j < 3; j++) {
                            try {
                                myclient.connect();
                                connected.add(topic);
                            } catch (Throwable e) {
                            }
                            if (myclient.isConnected() == false) {
                                try {
                                    Thread.sleep(j * 500);
                                } catch (Throwable e) {
                                }
                            } else
                                break;
                        }
                        if (myclient.isConnected() == false) {
                            failedCount++;
                        }
                    }
                }
                if (failedCount > 0) {
                    this.stats.setFailedCount(failedCount);
                    this.stats.setTopicCount(tlist.size() - failedCount);
                }

                for (MqttTopic topic : connected) {
                    topics.remove(topic);
                }
            }
            return;
        }
    }
}
