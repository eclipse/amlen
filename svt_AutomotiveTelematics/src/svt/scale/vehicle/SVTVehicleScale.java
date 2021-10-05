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

import java.util.Vector;

import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.util.SVTConfig;
import svt.util.SVTLog;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTConfig.IntegerEnum;
import svt.util.SVTConfig.LongEnum;
import svt.util.SVTConfig.StringEnum;
import svt.util.SVTLog.TTYPE;
//import svt.vehicle.SVTVehicle_MQTT;
import svt.vehicle.SVTVehicle_Paho;

//import com.ibm.micro.client.mqttv3.MqttMessage;
//import com.ibm.micro.client.mqttv3.MqttTopic;

public class SVTVehicleScale {
    static TTYPE TYPE = TTYPE.SCALE;
    Vector<Thread> list = null;
    Vector<SVTStats> statslist = null;
    static boolean stop = false;
    static int count = 0;
    static int threadcount = 0;
    public static long t = 0;
    int i = 0;
    int j = 0;
    int m = 0;
    int p = 0;
    String hostname = SVTUtil.getHostname();

    /**
     * @param args
     * @throws InterruptedException
     */
    public static void main(String[] args) {
            
        if (args.length == 0) {
            StringEnum.server.value = "9.3.179.214";
            IntegerEnum.port.value = 16102;
            StringEnum.appid.value = "1";
            IntegerEnum.jms.value = 100;
            IntegerEnum.mqtt.value = 100;
            IntegerEnum.paho.value = 0;
            LongEnum.minutes.value = 10L;
            StringEnum.mode.value = "connect_once";
            BooleanEnum.order.value = true;
            IntegerEnum.qos.value = 0;
        } else if (args[0].startsWith("-")) {
            SVTConfig.parse(args);
        } else if (args.length >= 8) {
            StringEnum.server.value = args[0];
            IntegerEnum.port.value = Integer.parseInt(args[1]);
            StringEnum.appid.value = args[2];
            IntegerEnum.jms.value = Integer.parseInt(args[3]);
            IntegerEnum.mqtt.value = Integer.parseInt(args[4]);
            IntegerEnum.paho.value = Integer.parseInt(args[5]);
            LongEnum.minutes.value = Long.parseLong(args[6]);
            StringEnum.mode.value = StringEnum.mode(Integer.parseInt(args[7]));
            if (args.length > 8) {
                BooleanEnum.order.value = Boolean.parseBoolean(args[8]);
            }
            if (args.length > 9) {
                IntegerEnum.qos.value = Integer.parseInt(args[9]);
            }
            if (args.length > 10) {
                BooleanEnum.stats.value = Boolean.parseBoolean(args[10]);
            }
            if (args.length > 11) {
                BooleanEnum.listener.value = Boolean.parseBoolean(args[11]);
            }
            if (args.length > 12) {
                StringEnum.userName.value = args[12];
                StringEnum.password.value = args[13];
            } 
        } else {
            SVTLog
                            .log3(TYPE,
                                            "arguments:  <ismserver> <ismport> <appId> <#JMS vehicles> <#MQTT vehicles> <#Paho vehicles> <#TimeToRun(minutes)>");
            System.exit(0);
        }

        SVTConfig.printSettings();

        // SVTLog.log(TYPE, "  ismserver: " + config.ismserver);
        // SVTLog.log(TYPE, "  ismport: " + config.ismport);
        // SVTLog.log(TYPE, "  VehicleTopic: " + config.appId);
        // SVTLog.log(TYPE, "  JMS vehicles: " + config.jmscars);
        // SVTLog.log(TYPE, "  MQTT vehicles: " + config.mqttcars);
        // SVTLog.log(TYPE, "  Paho vehicles: " + config.pahocars);
        // SVTLog.log(TYPE, "  TimeToRun: " + config.minutes + " minutes");
        // SVTLog.log(TYPE, "  Mode: " + config.mode + " 0:intermediate, 1:continuous");
        // SVTLog.log(TYPE, "  hostname: " + config.hostname);
        // SVTLog.log(TYPE, "  Order: " + config.order);
        // SVTLog.log(TYPE, "  Qos: " + config.qos);
        // SVTLog.log(TYPE, "  Listener: " + config.listener);
        // SVTLog.log(TYPE, "  ReportStats: " + config.reportstats);

        if (SVTConfig.help() == false) {
            SVTVehicleScale stress = new SVTVehicleScale();
            stress.begin();
        }

        System.exit(0);
    }

    public void begin() {

        list = new Vector<Thread>();
        statslist = new Vector<SVTStats>();
        SVTStats stats = null;
        
        String userPrefix = SVTUtil.getPrefix(StringEnum.userName.value);
        long userIndex = SVTUtil.getIndex(StringEnum.userName.value);
        int userIndexDigits = SVTUtil.getDigits(StringEnum.userName.value);
        String name = null;

        while ((j + m + p) < (IntegerEnum.jms.value + IntegerEnum.mqtt.value + IntegerEnum.paho.value)) {
            if (j < IntegerEnum.jms.value) {
                stats = new SVTStats(1000*60/IntegerEnum.rate.value, 1);
                statslist.add(stats);
                name = SVTUtil.formatName(userPrefix, userIndex, userIndexDigits);
                Thread t = new Thread(new SVTVehicleScale_JMSThread(StringEnum.mode.value, StringEnum.server.value,
                                IntegerEnum.port.value, StringEnum.appid.value, LongEnum.minutes.value,
                                LongEnum.hours.value, LongEnum.messages.value, stats, j + 1, BooleanEnum.order.value,
                                IntegerEnum.qos.value, BooleanEnum.listener.value, BooleanEnum.ssl.value, name,
                                StringEnum.password.value, BooleanEnum.genClientID.value));
                t.setName(name);
                userIndex++;
                list.add(t);
                j++;
            }
            if (m < IntegerEnum.mqtt.value) {
                stats = new SVTStats(1000*60/IntegerEnum.rate.value, 1);
                statslist.add(stats);
                name = SVTUtil.formatName(userPrefix, userIndex, userIndexDigits);
                Thread t = new Thread(new SVTVehicleScale_MQTTThread(StringEnum.mode.value, StringEnum.server.value,
                                IntegerEnum.port.value, StringEnum.appid.value, LongEnum.minutes.value,
                                LongEnum.hours.value, LongEnum.messages.value, stats, m + 1, BooleanEnum.order.value,
                                IntegerEnum.qos.value, BooleanEnum.listener.value, BooleanEnum.ssl.value, name,
                                StringEnum.password.value, BooleanEnum.genClientID.value));
                t.setName(name);
                list.add(t);
                userIndex++;
                m++;
            }
            if (p < IntegerEnum.paho.value) {
                stats = new SVTStats(1000*60/IntegerEnum.rate.value, 1);
                statslist.add(stats);
                name = SVTUtil.formatName(userPrefix, userIndex, userIndexDigits);
                Thread t = new Thread(new SVTVehicleScale_PahoThread(StringEnum.mode.value, StringEnum.server.value,
                		        StringEnum.server2.value,IntegerEnum.port.value, StringEnum.appid.value, LongEnum.minutes.value,
                                LongEnum.hours.value, LongEnum.messages.value, stats, p + 1, BooleanEnum.order.value,
                                IntegerEnum.qos.value, BooleanEnum.listener.value, IntegerEnum.listenerQoS.value, BooleanEnum.ssl.value, name,
                                StringEnum.password.value, IntegerEnum.keepAlive.value, IntegerEnum.connectionTimeout.value, BooleanEnum.cleanSession.value, BooleanEnum.genClientID.value));
                if (name != null) {
                	t.setName(name);
                } else {
                	t.setName("Unauth User");
                }
                list.add(t);
                userIndex++;
                p++;
            }
        }

        long starttime = System.currentTimeMillis();
        try {
            for (Thread t : list) {
                SVTUtil.threadcount(1);
                t.start();
                if (LongEnum.connectionThrottle.value > 0L){
               	 try {
	                  Thread.sleep(LongEnum.connectionThrottle.value);
                  } catch (InterruptedException e) {
                  }
                }
            }

            long lasttime = System.currentTimeMillis();

            while (activethreads(list)) {
                for (Thread t : list) {
                    while (t.isAlive()) {
                        t.join(60000);
                        lasttime = printStats(list, lasttime, StringEnum.server.value, StringEnum.server2.value, IntegerEnum.port.value,
                                        hostname, BooleanEnum.stats.value, BooleanEnum.ssl.value, t.getName(),
                                        StringEnum.password.value, IntegerEnum.keepAlive.value, IntegerEnum.connectionTimeout.value, BooleanEnum.cleanSession.value);
                    }
                }
            }
        } catch (Throwable e) {
            e.printStackTrace();
            System.exit(1);
        }

        long count = printFinalStats(starttime);
        if ((count > 0) && (SVTUtil.stop(null) == false)) {
            SVTLog.log3(TYPE, "SVTVehicleScale Success!!!");
        } else {
            SVTLog.log3(TYPE, "SVTVehicleScale Failed!!!");
        }
    }

    boolean activethreads(Vector<Thread> list) {
        boolean active = false;
        int i = 0;
        while ((active == false) && (i < list.size())) {
            active = list.elementAt(i).isAlive();
            i++;
        }
        return active;
    }

    SVTVehicle_Paho client = null;
    MqttTopic topic = null;

    long printStats(Vector<Thread> list, long lasttime, String ismserver, String ismserver2, Integer ismport, String hostname,
                    boolean reportstats, boolean ssl, String userName, String password, int keepAlive, int connectionTimeout, boolean cleanSession) {
        long time = System.currentTimeMillis();
        double delta_ms = time - lasttime;
        double delta_sec = delta_ms / 1000.0;
        // double delta_min = delta_sec / 60.0;
        long cars = 0;
        long count = 0;
        long failedCount = 0;
        for (SVTStats s : statslist) {
            count += s.getICount();
            cars += s.getTopicCount();
            failedCount += s.getFailedCount();
        }

        double rate_sec = 0;
        if (delta_sec > 0)
            rate_sec = count / delta_sec;

        // String text = String.format("%s%10.2f%s%10.2f%s", SVTUtil.MyPid() + ": " + cars + " clients on " + hostname +
        // " sent " + count
        // + " messages to " + ismserver + " in ", delta_min, " minutes.  The rate was ", rate_sec, " msgs/sec");
        // SVTLog.log3(TYPE, text);

        if (reportstats == true) {
            try {
                String clientId = SVTUtil.MyClientId(TYPE);
                if (topic == null) {
                	client = new SVTVehicle_Paho(ismserver, ismserver2, ismport, 0, 1, ssl, userName, password, keepAlive, connectionTimeout, cleanSession);
                    topic = client.getTopic(clientId, "/svt/" + clientId);
                }

                String text = clientId + ":" + cars + ":" + count + ":" + delta_ms + ":" + rate_sec + ":" + failedCount;
                // SVTLog.log3(TYPE, text);
                MqttMessage msg = new MqttMessage(text.getBytes());
                msg.setQos(1);
                topic.publish(msg);
            } catch (Throwable e) {
                SVTLog.logex(TYPE, "exception while publishing stats", e);
                topic = null;
            }
        }

        return time;
    }

    long printFinalStats(long starttime) {
        double delta_ms = System.currentTimeMillis() - starttime;
        double delta_sec = delta_ms / 1000.0;
        double delta_min = delta_sec / 60.0;
        long count = 0;
        long cars = 0;
        for (SVTStats s : statslist) {
            count += s.getCount();
            cars += s.getTopicCount();
        }

        double rate_min = 0;
        if (delta_min > 0)
            rate_min = count / delta_min;
        double rate_sec = 0;
        if (delta_sec > 0)
            rate_sec = count / delta_sec;

        SVTLog.log3(TYPE, String.format("%s%10.2f%s%10.2f%s%10.2f%s", SVTUtil.MyPid() + ": " + cars + " clients sent "
                        + count + " messages in ", delta_min, " minutes.  The rate was ", rate_min, " msgs/min, ",
                        rate_sec, " msgs/sec"));

        return count;
    }

}
