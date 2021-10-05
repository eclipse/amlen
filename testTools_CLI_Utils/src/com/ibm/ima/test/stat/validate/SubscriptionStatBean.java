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

package com.ibm.ima.test.stat.validate;

import org.supercsv.cellprocessor.ift.CellProcessor;



public class SubscriptionStatBean implements Comparable<SubscriptionStatBean> {

	private String subName = "";
	private String topicString = "";
	private String clientID = "";
	private String isDurable = "";
	private String bufferedMsgs = "";
	private String bufferedMsgsHWM = "";
	private String bufferedPercent = "";
	private String maxMessages = "";
	private String publishedMsgs = "";
	private String rejectedMsgs = "";
	private String bufferedHWMPercent = "";
	private String isShared = "";
	private String consumers = "";	
	private String discardedMsgs = "";
	private String expiredMsgs = "";
	private String messagingPolicy = "";

/*	
	SubName
	TopicString
	ClientID
	IsDurable
	BufferedMsgs
	BufferedMsgsHWM
	BufferedPercent
	MaxMessages
	DeliveredMsgs
	RejectedMsgs
	BufferedHWMPercent
	IsShared
	Consumers
	DiscardedMsgs
	ExpiredMsgs
	MessagingPolicy
	
	*/
	public static final CellProcessor[] subscriptionStatProcessor = new CellProcessor[] {
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null
	};
	
	public boolean equals(Object expected) {
		
		if (expected == null) {
			System.out.println("Entire stat entry line was empty.");
			return false;
		}
		
		
		if ( this == expected ) return true;
		
		 if ( !(expected instanceof SubscriptionStatBean) ) return false;
		
		 SubscriptionStatBean expectedBean = (SubscriptionStatBean) expected;

		
		boolean areEqual = true;
		
		if (subName.equals(expectedBean.getSubName()) == false) {
			System.out.println("subName not equal. Expected <" + expectedBean.getSubName() + "> " + "received <" + subName +">");
			areEqual = false;
		}
		
		if (topicString.equals(expectedBean.getTopicString()) == false) {
			System.out.println("topicString not equal. Expected <" +expectedBean.getTopicString() + "> " + "received <" + topicString +">");
			areEqual = false;
		}
		
		if (clientID.equals(expectedBean.getClientID()) == false) {
			System.out.println("clientID not equal. Expected <" +expectedBean.getClientID() + "> " + "received <" + clientID +">");
			areEqual = false;
		}
		
		//if (resetTime.equals(expectedBean.getResetTime()) == false) {
		//	System.out.println("resetTime not equal. Expected <" +expectedBean.getResetTime() + "> " + "received <" + resetTime +">");
		//	areEqual = false;
		//}
		
		if (isDurable.equals(expectedBean.getIsDurable()) == false) {
			System.out.println("isDurable not equal. Expected <" +expectedBean.getIsDurable() + "> " + "received <" + isDurable +">");
			areEqual = false;
		}
		
		if (bufferedMsgs.equals(expectedBean.getBufferedMsgs()) == false) {
			System.out.println("bufferedMsgs not equal. Expected <" +expectedBean.getBufferedMsgs() + "> " + "received <" + bufferedMsgs +">");
			areEqual = false;
		}
		
		if (bufferedMsgsHWM.equals(expectedBean.getBufferedMsgsHWM()) == false) {
			System.out.println("bufferedMsgsHWM not equal. Expected <" +expectedBean.getBufferedMsgsHWM() + "> " + "received <" + bufferedMsgsHWM +">");
			areEqual = false;
		}
		
		if (bufferedPercent.equals(expectedBean.getBufferedPercent()) == false) {
			System.out.println("bufferedPercent not equal. Expected <" +expectedBean.getBufferedPercent() + "> " + "received <" + bufferedPercent +">");
			areEqual = false;
		}
		
		if (maxMessages.equals(expectedBean.getMaxMessages()) == false) {
			System.out.println("maxMessages not equal. Expected <" +expectedBean.getMaxMessages() + "> " + "received <" + maxMessages +">");
			areEqual = false;
		}
		
		if (publishedMsgs.equals(expectedBean.getPublishedMsgs()) == false) {
			System.out.println("publishedMsgs not equal. Expected <" +expectedBean.getPublishedMsgs() + "> " + "received <" + publishedMsgs +">");
			areEqual = false;
		}
		
		if (rejectedMsgs.equals(expectedBean.getRejectedMsgs()) == false) {
			System.out.println("rejectedMsgs not equal. Expected <" +expectedBean.getRejectedMsgs() + "> " + "received <" + rejectedMsgs +">");
			areEqual = false;
		}
		
		if (bufferedHWMPercent.equals(expectedBean.getBufferedHWMPercent()) == false) {
			System.out.println("bufferedHWMPercent not equal. Expected <" +expectedBean.getBufferedHWMPercent() + "> " + "received <" + bufferedHWMPercent +">");
			areEqual = false;
		}

		if (isShared.equals(expectedBean.getIsShared()) == false) {
			System.out.println("isShared not equal. Expected <" +expectedBean.getIsShared() + "> " + "received <" + isShared +">");
			areEqual = false;
		}

		if (consumers.equals(expectedBean.getConsumers()) == false) {
			System.out.println("consumers not equal. Expected <" +expectedBean.getConsumers() + "> " + "received <" + consumers +">");
			areEqual = false;
		}

		if (discardedMsgs.equals(expectedBean.getDiscardedMsgs()) == false) {
			System.out.println("discardedMsgs not equal. Expected <" +expectedBean.getDiscardedMsgs() + "> " + "received <" + discardedMsgs +">");
			areEqual = false;
		}

		if (expiredMsgs.equals(expectedBean.getExpiredMsgs()) == false) {
			System.out.println("expiredMsgs not equal. Expected <" +expectedBean.getExpiredMsgs() + "> " + "received <" + expiredMsgs +">");
			areEqual = false;
		}
		
		if (messagingPolicy.equals(expectedBean.getMessagingPolicy()) == false) {
			System.out.println("messagingPolicy not equal. Expected <" +expectedBean.getMessagingPolicy() + "> " + "received <" + messagingPolicy +">");
			areEqual = false;
		}

		return areEqual;
		
	}

	public String getSubName() {
		return subName;
	}

	public void setSubName(String subName) {
		this.subName = subName;
	}

	public String getTopicString() {
		return topicString;
	}

	public void setTopicString(String topicString) {
		this.topicString = topicString;
	}

	public String getClientID() {
		return clientID;
	}

	public void setClientID(String clientID) {
		this.clientID = clientID;
	}

	public String getIsDurable() {
		return isDurable;
	}

	public void setIsDurable(String isDurable) {
		this.isDurable = isDurable;
	}

	public String getBufferedMsgs() {
		return bufferedMsgs;
	}

	public void setBufferedMsgs(String bufferedMsgs) {
		this.bufferedMsgs = bufferedMsgs;
	}

	public String getBufferedMsgsHWM() {
		return bufferedMsgsHWM;
	}

	public void setBufferedMsgsHWM(String bufferedMsgsHWM) {
		this.bufferedMsgsHWM = bufferedMsgsHWM;
	}

	public String getBufferedPercent() {
		return bufferedPercent;
	}

	public void setBufferedPercent(String bufferedPercent) {
		this.bufferedPercent = bufferedPercent;
	}

	public String getMaxMessages() {
		return maxMessages;
	}

	public void setMaxMessages(String maxMessages) {
		this.maxMessages = maxMessages;
	}

	public String getPublishedMsgs() {
		return publishedMsgs;
	}

	public void setPublishedMsgs(String publishedMsgs) {
		this.publishedMsgs = publishedMsgs;
	}

	public String getRejectedMsgs() {
		return rejectedMsgs;
	}

	public void setRejectedMsgs(String rejectedMsgs) {
		this.rejectedMsgs = rejectedMsgs;
	}

	public String getBufferedHWMPercent() {
		return bufferedHWMPercent;
	}

	public void setBufferedHWMPercent(String bufferedHWMPercent) {
		this.bufferedHWMPercent = bufferedHWMPercent;
	}

	public String getIsShared() {
		return isShared;
	}

	public void setIsShared(String isShared) {
		this.isShared = isShared;
	}

	public String getConsumers() {
		return consumers;
	}

	public void setConsumers(String consumers) {
		this.consumers = consumers;
	}

	public String getDiscardedMsgs() {
		return discardedMsgs;
	}

	public void setDiscardedMsgs(String discardedMsgs) {
		this.discardedMsgs = discardedMsgs;
	}
	
	public String getExpiredMsgs() {
		return expiredMsgs;
	}
	
	public void setExpiredMsgs(String expiredMsgs) {
		this.expiredMsgs = expiredMsgs;
	}

	public String getMessagingPolicy() {
		return messagingPolicy;
	}

	public void setMessagingPolicy(String messagingPolicy) {
		this.messagingPolicy = messagingPolicy;
	}
	
	@Override
	public int compareTo(SubscriptionStatBean o) {
		// TODO Auto-generated method stub
		String s1 = clientID + subName;
		String s2 = o.getClientID() + o.getSubName();
		return s1.compareTo(s2);
	}
	


}
