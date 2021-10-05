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

public class SubscriptionStatResult {

	private String subName = null;
	private String topicString = null;
	private String clientId = null;
	private boolean isDurable = false;
	private int bufferedMsgs = 0;
	private int bufferedMsgsHWM = 0;
	private double bufferedPercent=0.0;
	private int maxMessages =0;
	private int publishedMsgs = 0;
	private int rejectedMsgs = 0;
	private double bufferedHWMPercent = 0.0;
	private boolean isShared = false;
	private int consumers = 0;
	/*
	 * Below are new 1.2 stat output for Topics.
	 * Please add new stat output vars below for any 2.0 sprints 
	 */
	private int discardedmsgs = 0;
	private int expiredmsgs = 0;
	private String messagingpolicy = null;
	
	
	public SubscriptionStatResult(String subName, String topicString, String clientId , boolean isDurable, int bufferedMsgs,
			int bufferedMsgsHWM, double bufferedPercent, int maxMessages, int publishedMsgs, int rejectedMsgs, double bufferedHWMPercent, boolean isShared, int consumers, int discardedmsgs,int expiredmsgs,String messagingpolicy)
	{
		this.subName = subName;
		this.topicString = topicString;
		this.clientId = clientId;
		this.isDurable = isDurable;
		this.bufferedMsgs = bufferedMsgs;
		this.bufferedMsgsHWM = bufferedMsgsHWM;
		this.bufferedPercent = bufferedPercent;
		this.maxMessages = maxMessages;
		this.publishedMsgs = publishedMsgs;
		this.rejectedMsgs = rejectedMsgs;
		this.bufferedHWMPercent = bufferedHWMPercent;
		this.isShared = isShared;
		this.consumers = consumers;
		/*
		 * New stat output for 2.0.
		 * Please add new column vars below.
		 */
		this.discardedmsgs = discardedmsgs;
		this.expiredmsgs = expiredmsgs;
		this.messagingpolicy = messagingpolicy;
		
	}
	
	public String getSubName() {
		return subName;
	}
	public String getTopicString() {
		return topicString;
	}
	public String getclientId() {
		return clientId;
	}
	public boolean isDurable() {
		return isDurable;
	}
	public int getBufferedMsgs() {
		return bufferedMsgs;
	}
	public int getBufferedMsgsHWM() {
		return bufferedMsgsHWM;
	}
	public int getMaxMessages() {
		return maxMessages;
	}
	public int getPublishedMsgs() {
		return publishedMsgs;
	}
	public int getRejectedMsgs() {
		return rejectedMsgs;
	}
	
	public double getBufferedHWMPercent() {
		return bufferedHWMPercent;
	}

	public boolean isShared() {
		return isShared;
	}

	public int getConsumers() {
		return consumers;
	}
	/*
	 * New stat output for 2.0. 
	 * Please add new 2.0 stat functions below when new
	 * stat columms are added.
	 */
	public int getExpiredMsgs() {
		return expiredmsgs;
	}
	public int getDiscardedMsgs() {
		return discardedmsgs;
	}

	public String getMessagingPolicy() {
		return messagingpolicy;
	}
	
	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		
		buf.append("SubName=" + subName + "\n");
		buf.append("TopicString=" + topicString + "\n");
		buf.append("ClientId=" + clientId + "\n");
		buf.append("IsDurable=" + isDurable + "\n");
		buf.append("BufferedMsgs=" + bufferedMsgs + "\n");
		buf.append("BufferedMsgsHWM=" + bufferedMsgsHWM + "\n");
		buf.append("BufferedPercent=" + bufferedPercent + "\n");	
		buf.append("MaxMessages=" + maxMessages + "\n");
		buf.append("pPublishedMsgs=" + publishedMsgs + "\n");
		buf.append("RejectedMsgs=" + rejectedMsgs + "\n");
		buf.append("BufferedHWMPercent=" + bufferedHWMPercent + "\n");
		buf.append("IsShared=" + isShared + "\n");
		buf.append("Consumers=" + consumers + "\n");
		/*
		 * Please add new 2.0 stat output columns below
		 */
		buf.append("DiscardedMsgs=" + discardedmsgs + "\n");
		buf.append("ExpiredMsgs=" + expiredmsgs + "\n");
		buf.append("MessagingPolicy=" + messagingpolicy + "\n");
		
		return buf.toString();
	}

	
}
