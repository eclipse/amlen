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



public class TopicStatBean implements Comparable<TopicStatBean> {

	private String topicString = "";
	private String subscriptions = "";
	private String resetTime = "";
	private String publishedMsgs = "";
	private String rejectedMsgs = "";
	private String failedPublishes = "";

/*
	"TopicString",
	"Subscriptions",
	"ResetTime",
	"PublishedMsgs",
	"RejectedMsgs",
	"FailedPublishes"
*/
	public static final CellProcessor[] topicStatProcessor = new CellProcessor[] {
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
		
		 if ( !(expected instanceof TopicStatBean) ) return false;
		
		 TopicStatBean expectedBean = (TopicStatBean) expected;

		
		boolean areEqual = true;
		
		if (topicString.equals(expectedBean.getTopicString()) == false) {
			System.out.println("topicString not equal. Expected <" +expectedBean.getTopicString() + "> " + "received <" + topicString +">");
			areEqual = false;
		}
		
		if (subscriptions.equals(expectedBean.getSubscriptions()) == false) {
			System.out.println("subscriptions not equal. Expected <" +expectedBean.getSubscriptions() + "> " + "received <" + subscriptions +">");
			areEqual = false;
		}
		
		//if (resetTime.equals(expectedBean.getResetTime()) == false) {
		//	System.out.println("resetTime not equal. Expected <" +expectedBean.getResetTime() + "> " + "received <" + resetTime +">");
		//	areEqual = false;
		//}
		
		if (publishedMsgs.equals(expectedBean.getPublishedMsgs()) == false) {
			System.out.println("publishedMsgs not equal. Expected <" +expectedBean.getPublishedMsgs() + "> " + "received <" + publishedMsgs +">");
			areEqual = false;
		}
		
		if (rejectedMsgs.equals(expectedBean.getRejectedMsgs()) == false) {
			System.out.println("rejectedMsgs not equal. Expected <" +expectedBean.getRejectedMsgs() + "> " + "received <" + rejectedMsgs +">");
			areEqual = false;
		}
		
		if (failedPublishes.equals(expectedBean.getFailedPublishes()) == false) {
			System.out.println("failedPublishes not equal. Expected <" +expectedBean.getFailedPublishes() + "> " + "received <" + failedPublishes +">");
			areEqual = false;
		}
		
		
		
		return areEqual;
		
	}

	public String getTopicString() {
		return topicString;
	}

	public void setTopicString(String topicString) {
		this.topicString = topicString;
	}

	public String getSubscriptions() {
		return subscriptions;
	}

	public void setSubscriptions(String subscriptions) {
		this.subscriptions = subscriptions;
	}

	public String getResetTime() {
		return resetTime;
	}

	public void setResetTime(String resetTime) {
		this.resetTime = resetTime;
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

	public String getFailedPublishes() {
		return failedPublishes;
	}

	public void setFailedPublishes(String failedPublishes) {
		this.failedPublishes = failedPublishes;
	}

	@Override
	public int compareTo(TopicStatBean o) {
		return 0;
	}




}
