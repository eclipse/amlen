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
package com.ibm.ism.mqtt;

/**
 * This thread is responsible for keep alive timer in MQTT client.
 * Sends a PINGREQ to the server, if there was no inbound or outbound
 * activity.
 * 
 * If PINGRESP does not arrive within the keep alive interval after
 * PINGREQ, the TCP connection is terminated.
 */
public class IsmMqttKeepAliveThread implements Runnable {

	private boolean responseReceived;
	private boolean skipPing;
	private boolean stop;
	private IsmMqttConnection conn;
	private long lastInbound;
	private long lastOutbound;

	/**
	 * Constructs the thread object.
	 * @param connect The connection to MQTT server.
	 */
	public IsmMqttKeepAliveThread(IsmMqttConnection connect) {
		conn = connect;
		lastOutbound = System.currentTimeMillis();
		lastInbound = lastOutbound;
	}
	
	public void run() {
		if (conn.keepalive <= 0) {
			return;
		}
		
		long keepAlive = conn.keepalive * 1000;
		long startTime;
		long curTime;
		
		
		synchronized (this) {
			while (!stop) {
				skipPing = false;

				/* Wait before taking any action */
				startTime = System.currentTimeMillis();
				do {
					try {
						this.wait(keepAlive);
					} catch (InterruptedException e) {
					}

					if (stop || skipPing) {
						break;
					}

					curTime = System.currentTimeMillis();
				} while (curTime - startTime < keepAlive);
				
				/* If there was an inbound or outbound activity 
				 * during the wait period, keep waiting */
				if (stop || skipPing) {
					continue;
				}
				
				/* No activity, send PINGREQ */
				responseReceived = false;
				conn.sendPing();

				/* Wait for PINGRESP/keep alive timeout */
				startTime = System.currentTimeMillis();
				do {
					try {
						this.wait(keepAlive);
					} catch (InterruptedException e) {
					}

					if (stop || responseReceived) {
						break;
					}

					curTime = System.currentTimeMillis();
				} while (curTime - startTime < keepAlive);

				if (stop) {
					continue;
				}
				
				if (!responseReceived) {
					/* Disconnect socket */
					conn.terminateOnTimeout();
					break;
				}
			}
		}
	}
	
	/**
	 * Notifies the keep alive thread that PINGRESP has arrived.
	 */
	public void receivedPingResponse() {
		responseReceived = true;
		synchronized(this) {
			this.notify();
		}
	}

	/**
	 * Notifies the keep alive thread that there was an inbound message.
	 */
	public void inboundMessage() {
		long currentTime = System.currentTimeMillis();
		lastInbound = currentTime;
		if (currentTime < lastOutbound + conn.keepalive) {
			skipPing();
		}
	}

	/**
	 * Notifies the keep alive thread that there was an outbound message.
	 */
	public void outboundMessage() {
		long currentTime = System.currentTimeMillis();
		lastOutbound = currentTime;
		if (currentTime < lastInbound + conn.keepalive) {
			skipPing();
		}
	}

	/**
	 * Asks the keep alive thread to skip sending PINGREQ.
	 * This can happen if there was both inbound and 
	 * outbound activity during the interval.
	 */
	private void skipPing() {
		skipPing = true;
		synchronized(this) {
			this.notify();
		}
	}
	
	/**
	 * Asks the keep alive thread to stop.
	 */
	public void stop() {
		stop = true;
		synchronized(this) {
			this.notify();
		}
	}
	
}
