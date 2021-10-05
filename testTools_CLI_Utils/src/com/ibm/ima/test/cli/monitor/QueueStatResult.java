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
package com.ibm.ima.test.cli.monitor;

public class QueueStatResult {

	private String queueName = null;
	private int producers = 0;
	private int consumers = 0;
	private int bufferedMsgs = 0;
	private int bufferedMsgsHWM = 0;
	private double bufferedPercent = 0;
	private int maxMessages = 0;
	private int producedMsgs = 0;
	private int consumedMsgs = 0;
	private int rejectedMsgs = 0;
	private double bufferedHWMPercent = 0.0;
	
	public QueueStatResult(String queueName, int producers, int consumers, 
			int bufferredMsgs, int bufferedMsgsHWM, double bufferedPercent, 
			int maxMessages, int producedMsgs, int consumedMsgs, int rejectedMsgs, double bufferedHWMPercent)
	{
		this.queueName = queueName;
		this.producers = producers;
		this.consumers = consumers;
		this.bufferedMsgs = bufferredMsgs;
		this.bufferedMsgsHWM = bufferedMsgsHWM;
		this.bufferedPercent = bufferedPercent;
		this.maxMessages = maxMessages;
		this.producedMsgs = producedMsgs;
		this.consumedMsgs = consumedMsgs;
		this.rejectedMsgs = rejectedMsgs;
		this.bufferedHWMPercent = bufferedHWMPercent;
	}
	
	public String getQueueName() {
		return queueName;
	}
	
	public int getProducers() {
		return producers;
	}
	
	public int getConsumers() {
		return consumers;
	}
	
	public int getBufferedMsgs() {
		return bufferedMsgs;
	}
	
	public int getBufferedMsgsHWM() {
		return bufferedMsgsHWM;
	}
	
	public double getBufferedPercent() {
		return bufferedPercent;
	}
	
	public int getMaxMessages() {
		return maxMessages;
	}
	
	public int getProducedMsgs() {
		return producedMsgs;
	}
	
	public int getConsumedMsgs() {
		return consumedMsgs;
	}
	
	public int getRejectedMsgs() {
		return rejectedMsgs;
	}
	
	public double getBufferedHWMPercent() {
		return bufferedHWMPercent;
	}
	
	
	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		buf.append("QueueName=" + queueName + "\n");
		buf.append("Producers=" + producers + "\n");
		buf.append("Consumers=" + consumers + "\n");
		buf.append("BufferedMsgs=" + bufferedMsgs + "\n");
		buf.append("BufferedMsgsHWM=" + bufferedMsgsHWM + "\n");
		buf.append("BufferedPercent=" + bufferedPercent + "\n");
		buf.append("ProducedMsgs=" + producedMsgs + "\n");
		buf.append("ConsumedMsgs=" + consumedMsgs + "\n");
		buf.append("RejectedMsgs=" + rejectedMsgs + "\n");
		buf.append("BfferedHWMPercent=" + bufferedHWMPercent + "\n");
		
		
		
		
		return buf.toString();
	}
	
}
