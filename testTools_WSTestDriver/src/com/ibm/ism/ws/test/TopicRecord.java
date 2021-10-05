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
package com.ibm.ism.ws.test;

import java.text.ParseException;
import java.util.Date;

public class TopicRecord extends MonitorRecord {
	public final String		topicString;
	public final long		subscriptions;
	public final Date		resetTime;
	public final long		publishedMsgs;
	public final long		rejectedMsgs;
	public final long		failedPublishes;

	TopicRecord(ImaJson json) throws IsmTestException  {
		super(json);
		
		// Parse the rest
		topicString = json.getString("TopicString");
		if (null == topicString) throw new IsmTestException("ISMTEST"+(Constants.TOPICRECORD),
			"TopicString not in json data");
		subscriptions = Long.valueOf(json.getString("Subscriptions"));
		publishedMsgs = Long.valueOf(json.getString("PublishedMsgs"));
		rejectedMsgs = Long.valueOf(json.getString("RejectedMsgs"));
		failedPublishes = Long.valueOf(json.getString("FailedPublishes"));
		String tStamp = json.getString("ResetTime");
		try {
			resetTime = TestUtils.parse8601(tStamp);
		} catch (ParseException pe) {
			throw new IsmTestException("ISMTEST"+(Constants.MONITORRECORD),
				"Parse Exception thrown trying to parse ResetTime '"+tStamp+"'", pe);
		}
		//
	}
	
	public boolean validateValue(String name, String op, String value) throws IsmTestException {
		long myValue = 0;
		if ("PublishedMsgs".equals(name)) {
			myValue = publishedMsgs;
		} else if ("Subscriptions".equals(name)) {
			myValue = subscriptions;
		} else if ("RejectedMsgs".equals(name)) {
			myValue = rejectedMsgs;
		} else if ("FailedPublishes".equals(name)) {
			myValue = failedPublishes;
		} else if ("TopicString".equals(name)) {
			if (value.equals(topicString)) {
				return true;
			} else {
				return false;
			}
		} else {
			throw new IsmTestException("ISMTEST"+(Constants.TOPICRECORD+1),
			"Value "+name+" is not supported for comparisons");
		}
		TestUtils.CompareType compareType = TestUtils.CompareType.valueOf(op);
		switch (compareType) {
		case EQ:
			return myValue == Integer.valueOf(value);
		case NE:
			return myValue != Integer.valueOf(value);
		case GT:
			return myValue > Integer.valueOf(value);
		case GE:
			return myValue >= Integer.valueOf(value);
		case LT:
			return myValue < Integer.valueOf(value);
		case LE:
			return myValue <= Integer.valueOf(value);
		}
		return false;
	}
}
