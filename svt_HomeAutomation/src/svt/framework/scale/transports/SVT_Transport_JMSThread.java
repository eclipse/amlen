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

import javax.jms.Connection;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import svt.framework.device.SVT_Device;
import svt.framework.messages.SVT_BaseEvent;
import svt.framework.transports.SVT_Transport;
import svt.framework.transports.SVT_Transport_JMS;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVT_Params;

public class SVT_Transport_JMSThread extends SVT_Transport_Thread implements
		Runnable {
	static TTYPE TYPE = TTYPE.ISMJMS;

    Set<Topic> failures = new HashSet<Topic>();

    Hashtable<Topic, Connection> tlist;
    
	public SVT_Transport_JMSThread(SVT_Params parms) {
		super(parms);
	}

	public boolean processCommand(String topic, String command){
		return true;
	}
	
	public void run() {
		String pClientId = SVTUtil.MyClientId(TYPE);
		this.topicName = SVTUtil.MyTopicId(appId, pClientId);
		String message = "";
		int i=0;
		// SVTLog.log(TYPE, "thread created");
		
		SVT_Params parms = new SVT_Params("0", ismserver, "", ismport, "",
				0L, 0L, 0L, (SVTStats) null, threadId, false, 0, false, ssl,
				userName, password,false,0);
		SVT_Transport_JMS vehicle = (SVT_Transport_JMS)createTransport(parms);
		;
		if (this.listener == true)
			vehicle.startListener();

		try {
			this.stats.reinit();

			SVTLog.log3(TYPE, pClientId + " to send messages on topic "
					+ topicName + " Connection mode is " + mode);

			if ("connect_each".equals(mode)) {
				while (true) {
					this.stats.beforeMessage();
					if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
							|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
						break;
					}
					int x = (int) (Math.random() * 1000.0);
					int y = (int) (Math.random() * 1000.0);
					vehicle.sendMessage(topicName, pClientId, "GPS " + x + ", "
							+ y + ")", i++);
					this.stats.increment();
					if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
							|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
						break;
					}
					this.stats.afterMessageRateControl();
				}
			} else if ("connect_once".equals(mode)) {
				Session session = vehicle.getSession(pClientId);
				MessageProducer theProducer = vehicle.getProducer(session,
						topicName);

				while (true) {
					this.stats.beforeMessage();
					if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
							|| ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
						break;
					}
					int x = (int) (Math.random() * 1000.0);
					int y = (int) (Math.random() * 1000.0);
					vehicle.sendMessage(theProducer, session, "GPS " + x + ", "
							+ y + ")", i++);
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
			SVTLog.logex(TYPE, "sendMessage", e);
			SVTUtil.stop(true);
		}
		if (this.listener == true)
			vehicle.stopListener();

		int t = SVTUtil.threadcount(-1);

		String text = "jmsclient ," + pClientId + ", finished__" + message
				+ ". Ran for ," + this.stats.getRunTimeSec() + ", sec. Sent ,"
				+ i + ", messages. Rate: ," + this.stats.getRatePerMin()
				+ ", msgs/min ," + this.stats.getRatePerSec() + ", msgs/sec "
				+ t + " clients remain.";

		SVTLog.log3(TYPE, text);
		return;
	}


	@Override
	public void startThread() {
		
	}

	@Override
	public void stopThread() {
		
	}

	@Override
	public boolean sendEvent(SVT_BaseEvent event) {
		return true;
	}

	@Override
	protected SVT_Transport_JMS createTransport(SVT_Params parms) {
			SVT_Transport_JMS transport= new SVT_Transport_JMS(parms,this);
			return transport;
	}

	@Override
	protected void connectBatchWithOrder(String clientId,
			SVT_Transport transport) throws Exception, InterruptedException {
		
	}

	@Override
	protected void connectBatch(String clientId, SVT_Transport transport)
			throws Exception, Throwable, InterruptedException {
		
	}

	@Override
	protected void connectOnce(String clientId, SVT_Transport transport)
			throws Exception, Throwable, InterruptedException {
		
	}

	@Override
	protected void connectEach(String clientId, SVT_Transport transport)
			throws Throwable, InterruptedException {
		
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

	@Override
	public void addDevice(SVT_Device dev) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void addListenerTopic(String topic) {
	}

}
