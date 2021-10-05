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

import java.util.Date;


public class TopicStatResult {
	
	private String topicString = null;
	private int subscriptions = 0;
	private Date resetTime = null;
	private int publishedMsgs = 0;
	private int rejectedMsgs = 0;
	private int failedPublishes = 0;
	
	
	public TopicStatResult(String topicString, int subscriptions, Date resetTime, 
			int publishedMsgs, int rejectedMsgs, int failedPublishes)
	{
		this.topicString = topicString;
		this.subscriptions = subscriptions;
		this.resetTime = resetTime;
		this.publishedMsgs = publishedMsgs;
		this.rejectedMsgs = rejectedMsgs;
		this.failedPublishes = failedPublishes;
	}
	
	
	public String getTopicString() {
		return topicString;
	}
	
	public int getSubscriptions() {
		return subscriptions;
	}
	
	public Date getResetTime() {
		return resetTime;
	}
	
	public int getPublishedMsgs() {
		return publishedMsgs;
	}
	
	public int getRejectedMsgs() {
		return rejectedMsgs;
	}
	
	public int getFailedPublishes() {
		return failedPublishes;
	}
	
	
	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		buf.append("TopicString=" + topicString + "\n");
		buf.append("Subscriptions=" + subscriptions + "\n");
		buf.append("ResetTime=" + resetTime + "\n");
		buf.append("PublishedMsgs=" + publishedMsgs + "\n");
		buf.append("RejectedMsgs=" + rejectedMsgs + "\n");
		buf.append("FailedPublishes=" + failedPublishes + "\n");
		
		return buf.toString();
	}
}
