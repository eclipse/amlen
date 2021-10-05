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

import svt.framework.device.SVT_Device;
import svt.framework.messages.SVT_BaseEvent;
import svt.framework.transports.SVT_Transport;
import svt.util.SVTStats;
import svt.util.SVT_Params;

public abstract class SVT_Transport_Thread implements Runnable {
	String mode = "connect_once";
	String ismserver = "";
	String ismserver2 = "";
	Integer ismport = null;
	public String topicName = "";
	String appId = "";
	long minutes = 0;
	long messages = 0;
	long max = 0;
	SVTStats stats = null;
	// Vector<Thread> list = null;
	// Vector<SVTStats> statsList = null;
	long threadId = 0;
	boolean order = true;
	Integer qos = 0;
	boolean listener = false;
	boolean ssl = false;
	String userName = null;
	String password = null;
	String listenerTopic = null;

	public SVT_Transport_Thread(SVT_Params parms) {
		this.mode = parms.mode;
		this.ismserver = parms.ismserver;
		this.ismserver2 = parms.ismserver2;
		this.ismport = parms.ismport;
		this.appId = parms.appId;
		if (parms.minutes != null) {
			this.minutes = parms.minutes;
		}
		if (parms.hours != null) {
			this.minutes += (parms.hours * 60);
		}
		if (parms.messages != null) {
			this.messages = parms.messages;
		}
			this.max = this.minutes * 60000;
		this.stats = parms.stats;
		this.threadId = parms.threadId;
		this.order = parms.order;
		this.qos = parms.qos;
		this.listener = parms.listener;
		this.ssl = parms.ssl;
		this.userName = parms.userName;
		this.password = parms.password;
	}

	public void run() {
		return;
	}

	public abstract void addDevice(SVT_Device dev);
	public void setListenerTopic(String topic) {
		listenerTopic = topic;
	}
	public abstract void addListenerTopic(String topic);

	public abstract boolean processCommand(String topic, String command);

	public abstract void startThread();

	public abstract void stopThread();

	public abstract boolean sendEvent(SVT_BaseEvent event);

	protected abstract SVT_Transport createTransport(SVT_Params parms);

	protected abstract void connectBatchWithOrder(String clientId,
			SVT_Transport transport) throws Exception, Throwable,
			InterruptedException;

	protected abstract void connectBatch(String clientId,
			SVT_Transport transport) throws Exception, Throwable,
			InterruptedException;

	protected abstract void connectOnce(String clientId, SVT_Transport transport)
			throws Exception, Throwable, InterruptedException;

	protected abstract void connectEach(String clientId, SVT_Transport transport)
			throws Throwable, InterruptedException;

}
