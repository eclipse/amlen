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

import javax.jms.Connection;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import svt.util.SVTLog;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVTLog.TTYPE;
import svt.vehicle.SVTVehicle_JMS;

public class SVTVehicleScale_JMSThread extends SVTVehicleScale implements Runnable {
    static TTYPE TYPE = TTYPE.ISMJMS;
    String mode = "connect_once";
    String ismserver = "";
    Integer ismport = null;
    String topicName = "";
    String appId = "";
    long minutes = 0;
    long messages = 0;
    long max = 0;
    long i = 0;
    SVTStats stats = null;
    Hashtable<Topic, Connection> tlist;
    long carId = 0;
    boolean order = true;
    Integer qos = 0;
    Set<Topic> failures = new HashSet<Topic>();
    boolean listener = false;
    boolean ssl = false;
    String userName = null;
    String password = null;
    boolean genClientId = true;

    SVTVehicleScale_JMSThread(String mode, String ismserver, Integer ismport, String appId, Long minutes, Long hours,
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

        SVTVehicle_JMS vehicle = new SVTVehicle_JMS(ismserver, ismport, carId, ssl, userName, password);
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
                    vehicle.sendMessage(topicName, pClientId, "GPS " + x + ", " + y + ")", i++);
                    this.stats.increment();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    this.stats.afterMessageRateControl();
                }
            } else if ("connect_once".equals(mode)) {
                Session session = vehicle.getSession(pClientId);
                MessageProducer theProducer = vehicle.getProducer(session, topicName);

                while (true) {
                    this.stats.beforeMessage();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    int x = (int) (Math.random() * 1000.0);
                    int y = (int) (Math.random() * 1000.0);
                    vehicle.sendMessage(theProducer, session, "GPS " + x + ", " + y + ")", i++);
                    this.stats.increment();
                    if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                                    || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                        break;
                    }
                    this.stats.afterMessageRateControl();
                }
                vehicle.close(theProducer, session);
            } else if ("batch".equals(mode)) {
            } else if ("batch_with_order".equals(mode)) {
            }
        } catch (Throwable e) {
            // if (e.getMessage() == null) {
            // SVTLog
            // .log(TYPE,
            // "Exception caught... una.");
            // SVTLog.log(TYPE, "Free Memory:  "
            // + Runtime.getRuntime().freeMemory());
            // SVTLog.log(TYPE, "Maximum Memory:  "
            // + Runtime.getRuntime().maxMemory());
            // SVTLog.log(TYPE, "Total Memory:  "
            // + Runtime.getRuntime().totalMemory());
            //
            // /* Get a list of all filesystem roots on this system */
            // File[] roots = File.listRoots();
            //
            // /* For each filesystem root, print some info */
            // for (File root : roots) {
            // SVTLog.log(TYPE, "\nFile system root: "
            // + root.getAbsolutePath());
            // SVTLog.log(TYPE, "Total space (bytes): "
            // + root.getTotalSpace());
            // SVTLog.log(TYPE, "Free space (bytes): "
            // + root.getFreeSpace());
            // SVTLog.log(TYPE, "Usable space (bytes): "
            // + root.getUsableSpace() + "\n");
            // }
            //
            // } else {
            SVTLog.logex(TYPE, "sendMessage", e);
            SVTUtil.stop(true);
        }
        if (this.listener == true)
            vehicle.stopListener();

        int t = SVTUtil.threadcount(-1);

        String text = "jmsclient ," + pClientId + ", finished__" + message + ". Ran for ," + this.stats.getRunTimeSec()
                        + ", sec. Sent ," + i + ", messages. Rate: ," + this.stats.getRatePerMin() + ", msgs/min ,"
                        + this.stats.getRatePerSec() + ", msgs/sec " + t + " clients remain.";

        SVTLog.log3(TYPE, text);
        return;
    }

    class failuresThread implements Runnable {
        Connection myclient = null;
        Set<Topic> topics = new HashSet<Topic>();
        Set<Topic> connected = new HashSet<Topic>();
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
                for (Topic topic : topics) {
                    myclient = tlist.get(topic);
                    for (int j = 0; j < 3; j++) {
                        try {
                            myclient.start();
                            connected.add(topic);
                            break;
                        } catch (Throwable e) {
                            failedCount++;
                            try {
                                Thread.sleep(j * 500);
                            } catch (Throwable e2) {
                            }
                        }
                    }
                    if (failedCount > 0) {
                        this.stats.setFailedCount(failedCount);
                        this.stats.setTopicCount(tlist.size() - failedCount);
                    }
                }
                for (Topic topic : connected) {
                    topics.remove(topic);
                }
            }
            return;
        }
    }
}
