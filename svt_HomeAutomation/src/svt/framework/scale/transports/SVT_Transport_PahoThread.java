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
package svt.framework.scale.transports;

import java.util.HashSet;
import java.util.Hashtable;
import java.util.Set;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.framework.device.SVT_Device;
import svt.framework.messages.SVT_BaseEvent;
import svt.framework.transports.SVT_Transport;
import svt.framework.transports.SVT_Transport_Paho;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVT_Params;

public class SVT_Transport_PahoThread extends SVT_Transport_Thread implements
		Runnable {

	static TTYPE TYPE = TTYPE.PAHO;

	Set<MqttTopic> failures = new HashSet<MqttTopic>();
	Hashtable<MqttTopic, MqttClient> tlist;
	SVT_Transport_Paho transportClient;
	
	public SVT_Transport_PahoThread(SVT_Params parms) {
		super(parms);
	}

	SVT_Device myDevice;

	@Override
	public void addDevice(SVT_Device dev) {
		if (myDevice == null) {
			myDevice = dev;
			myDevice.setThread(this);
			this.topicName=myDevice.getTopic();
		} else {
			myDevice.setThread(null);
			myDevice = dev;
			myDevice.setThread(this);
			this.topicName=myDevice.getTopic();
		}

	}

	public boolean processCommand(String topic, String command) {
		return myDevice.processCommand(topic, command);
	}

	@Override
	public void startThread() {
		// TODO Auto-generated method stub

	}

	@Override
	public void stopThread() {
		// TODO Auto-generated method stub

	}

	@Override
	public boolean sendEvent(SVT_BaseEvent event) {
		transportClient.sendMessage(event);
		return true;
	}


	public void run() {
		String pClientId = SVTUtil.MyClientId(TYPE);
		if (this.topicName == null) {
			this.topicName = SVTUtil.MyTopicId(appId, pClientId);
		}
		String message = "";
		// SVTLog.log(TYPE, "thread created");
		SVT_Params parms = new SVT_Params("0", ismserver, ismserver2, ismport,
				"", 0L, 0L, 0L, (SVTStats) null, threadId, false, qos, false,
				ssl, userName, password,false, 0);
		transportClient = createTransport(parms);
		transportClient.setListenerTopic(this.topicName);
		if (this.listener == true)
			transportClient.startListener();

		try {
			this.stats.reinit();

			SVTLog.log3(TYPE, pClientId + " to send messages on topic "
					+ topicName + " Connection mode is " + mode);

			if ("connect_each".equals(mode)) {
				connectEach(pClientId, transportClient);
			} else if ("connect_once".equals(mode)) {
				connectOnce(pClientId, transportClient);
			} else if ("batch".equals(mode)) {

				connectBatch(pClientId, transportClient);
			} else if ("batch_with_order".equals(mode)) {
				connectBatchWithOrder(pClientId, transportClient);
			}

		} catch (Throwable e) {
			SVTLog.logex(TYPE, "sendMessage", e);
			SVTUtil.stop(true);
		}

		if (this.listener == true)
			transportClient.stopListener();

		int t = SVTUtil.threadcount(-1);

		String text = "mqttclient ," + pClientId + ", finished__" + message
				+ ". Ran for ," + this.stats.getRunTimeSec() + ", sec. Sent ,"
				+ this.stats.getCount() + ", messages. Rate: ,"
				+ this.stats.getRatePerMin() + ", msgs/min ,"
				+ this.stats.getRatePerSec() + ", msgs/sec " + t
				+ " clients remain.";

		SVTLog.log3(TYPE, text);
		return;
	}

	public SVT_Transport_Paho createTransport(SVT_Params parms) {

		SVT_Transport_Paho transport = new SVT_Transport_Paho(parms, this);
		return transport;
	}

	protected void connectBatchWithOrder(String clientId,
			SVT_Transport transport) throws MqttException,
			Throwable, InterruptedException {
		int i;
		MqttClient myclient = null; 
		MqttTopic mytopic = null; 
		int tlistsize = 50;
		tlist = new Hashtable<MqttTopic, MqttClient>();
		this.stats.setTopicCount(tlistsize);
		for (i = 0; i < tlistsize; i++) {
			myclient = ((SVT_Transport_Paho)transport).getClient(clientId + "_" + i);
			mytopic = myclient.getTopic(topicName + "_" + i);
			tlist.put(mytopic, myclient);
		}

		MqttMessage textMessage = null;

		Set<MqttTopic> topicSet = tlist.keySet();

		if (this.order == false) {
			for (MqttTopic topic : topicSet) {
				String text = getMessage();
				try {
					((SVT_Transport_Paho)transport).sendMessage(topic, text, this.qos);
				} catch (MqttException e) {
					failures.add(topic);
				}
			}
		}

		while (true) {
			if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
					|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
				break;
			}
			this.stats.beforeMessage();
			for (MqttTopic topic : topicSet) {
				if (this.order == true) {
					String text = "ORDER:" + tlist.get(topic).getClientId()
							+ ":" + i + ":" + getMessage();
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

	protected void connectBatch(String clientId, SVT_Transport transport)
			throws MqttException, Throwable, InterruptedException {
		MqttClient client = ((SVT_Transport_Paho)transport).getClient(clientId);
		MqttTopic topic = client.getTopic(topicName);
		boolean sent = false;
		while (true) {
			this.stats.beforeMessage();
			if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
					|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
				break;
			}
			sent = false;
			while (sent == false) {
				if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
						|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
					break;
				}
				try {
					if (topic == null) {
						client = ((SVT_Transport_Paho)transport).getClient(clientId);
						topic = client.getTopic(topicName);
					}
					((SVT_Transport_Paho)transport).sendMessage(topic, getMessage());
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
			SVTLog.logex(TYPE, clientId
					+ ": Exception caught from client.disconnect()", e);
		}
	}

	protected  void connectOnce(String clientId, SVT_Transport transport)
			throws MqttException, Throwable, InterruptedException {
		MqttClient client = ((SVT_Transport_Paho)transport).getClient(clientId);
		MqttTopic topic = client.getTopic(topicName);
		while (true) {
			this.stats.beforeMessage();
			if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
					|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
				break;
			}
			((SVT_Transport_Paho)transport).sendMessage(topic, getMessage());
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
			SVTLog.logex(TYPE, clientId
					+ ": Exception caught from client.disconnect()", e);
		}
	}

	protected void connectEach(String clientId, SVT_Transport transport)
			throws Throwable, InterruptedException {
		while (true) {
			this.stats.beforeMessage();
			if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
					|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
				break;
			}
			((SVT_Transport_Paho)transport).sendMessage(topicName, clientId, getMessage(),
					this.stats.increment());
			if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
					|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
				break;
			}
			this.stats.afterMessageRateControl();
		}
	}

	private String getMessage() {
		String returnString = "";
		if (myDevice != null) {
			returnString = "" + myDevice.getCurrentValue();
		}
		return returnString;
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

	@Override
	public void addListenerTopic(String topic) {
		transportClient.addListenerTopic(topic);
	}

}
