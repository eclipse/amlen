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
package svt.framework.scale;

import java.util.Vector;

import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.framework.transports.SVT_Transport_Paho;
import svt.util.SVTConfig;
import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTConfig.IntegerEnum;
import svt.util.SVTConfig.LongEnum;
import svt.util.SVTConfig.StringEnum;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
//import svt.vehicle.SVTVehicle_MQTT;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVT_Params;

//import com.ibm.micro.client.mqttv3.MqttMessage;
//import com.ibm.micro.client.mqttv3.MqttTopic;

public class SVT_Engine {
	protected static TTYPE TYPE = TTYPE.SCALE;
	protected Vector<Thread> list = null;
	protected Vector<SVTStats> statslist = null;
	static boolean stop = false;
	static int count = 0;
	static int threadcount = 0;
	public static long t = 0;
	protected int j = 0;
	protected int m = 0;
	protected int p = 0;
	protected String hostname = SVTUtil.getHostname();
	SVT_Transport_Paho client = null; 
	MqttTopic topic = null;
	SVT_Params myParms;

	public SVT_Engine() {
		SVTStats stats = new SVTStats(1000 * 60 / IntegerEnum.rate.value, 1);

		myParms = new SVT_Params(StringEnum.mode.value,
				StringEnum.server.value, StringEnum.server2.value,
				IntegerEnum.port.value, StringEnum.appid.value,
				LongEnum.minutes.value, LongEnum.hours.value,
				LongEnum.messages.value, stats, 0L, BooleanEnum.order.value,
				IntegerEnum.qos.value, BooleanEnum.listener.value,
				BooleanEnum.ssl.value, StringEnum.userName.value,
				StringEnum.password.value,BooleanEnum.cleanSession.value,0);
	}

	/**
	 * @param args
	 * @throws InterruptedException
	 */
	public static void main(String[] args) {

		if (args.length == 0) {
			SVTLog.log3(TYPE, "No arguments supplied.");

			System.exit(0);
		} else {
			SVTConfig.parse(args);
		}
		SVTConfig.printSettings();

		if (SVTConfig.help() == false) {
			SVT_Engine stress = new SVT_Engine();
			stress.begin();
		}

		System.exit(0);
	}

	public void begin() {
	}

	protected boolean activethreads(Vector<Thread> list) {
		boolean active = false;
		int i = 0;
		while ((active == false) && (i < list.size())) {
			active = list.elementAt(i).isAlive();
			i++;
		}
		return active;
	}

	protected long printStats(Vector<Thread> list, long lasttime,
			String ismserver, String ismserver2, Integer ismport,
			String hostname, boolean reportstats, boolean ssl, String userName,
			String password) {
		long time = System.currentTimeMillis();
		String clientId = SVTUtil.MyClientId(TYPE);
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

		// String text = String.format("%s%10.2f%s%10.2f%s", SVTUtil.MyPid() +
		// ": " + cars + " clients on " + hostname +
		// " sent " + count
		// + " messages to " + ismserver + " in ", delta_min,
		// " minutes.  The rate was ", rate_sec, " msgs/sec");
		// SVTLog.log3(TYPE, text);
		SVT_Params parms = new SVT_Params("0", ismserver, ismserver2, ismport,
				"", 0L, 0L, 0L, (SVTStats) null, 0L, false, 1, false, ssl,
				userName, password,false,0);
		if (reportstats == true) {
			try {
				if (topic == null) {
					client = new SVT_Transport_Paho(parms, null);
					topic = client.getTopic(clientId, "/svt/" + clientId);
				}

				String text = clientId + ":" + cars + ":" + count + ":"
						+ delta_ms + ":" + rate_sec + ":" + failedCount;
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

	protected long printFinalStats(long starttime) {
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

		SVTLog.log3(TYPE, String.format("%s%10.2f%s%10.2f%s%10.2f%s",
				SVTUtil.MyPid() + ": " + cars + " clients sent " + count
						+ " messages in ", delta_min,
				" minutes.  The rate was ", rate_min, " msgs/min, ", rate_sec,
				" msgs/sec"));

		return count;
	}

}
