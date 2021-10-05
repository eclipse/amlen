package com.ibm.ism.test.testcases.messaging.messagingpolicies;


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
import com.ibm.ism.test.common.IsmTestData;

public class ImsCreateMessagingPoliciesTestData extends IsmTestData {

	public static final String KEY_MESSAGE_HUB = "MESSAGE_HUB";
	public static final String KEY_MESSAGING_POLICY_NAME = "MESSAGING_POLICY_NAME";
	public static final String KEY_MESSAGING_POLICY_DESC = "MESSAGING_POLICY_DESC";
	public static final String KEY_DESTINATION = "DESTINATION";
	public static final String KEY_DESTINATION_TYPE = "DESTINATION_TYPE";
	public static final String KEY_MAX_MESSAGES = "MAX_MESSAGES";
	public static final String KEY_MAX_MESSAGES_BEHAVIOR_TYPE = "MAX_MESSAGES_BEHAVIOR_TYPE";
	public static final String KEY_USER_ID = "USER_ID";
	public static final String KEY_CLIENT_ADDRESS = "CLIENT_ADDRESS";
	public static final String KEY_COMMON_NAME = "COMMON_NAME";
	public static final String KEY_CLIENT_ID = "CLIENT_ID";
	public static final String KEY_GROUP_ID = "GROUP_ID";
	public static final String KEY_MAX_MESSAGE_TTL = "MAX_MESSAGE_TTL";
	public static final String KEY_DISCONNECTED_CLIENT_NOTIFICATION = "DISCONNECTED_CLIENT_NOTIFICATION";
	
	public static final String DESTINATION_TYPE_TOPIC = "Topic";
	public static final String DESTINATION_TYPE_QUEUE = "Queue";
	public static final String DESTINATION_TYPE_SUBSCRIPTION = "Global-sharedSubscription";

	public static final String MAX_MESSAGES_BEHAVIOR_TYPE_REJECT = "REJECT";
	public static final String MAX_MESSAGES_BEHAVIOR_TYPE_DISCARD = "DISCARD";
	
	
	public ImsCreateMessagingPoliciesTestData(String[] args) {
		super(args);
	}
	
	public String getMessageHub() {
		return getProperty(KEY_MESSAGE_HUB);
	}
	
	public String getNewMessagingPoliciesID() {
		return getProperty(KEY_MESSAGING_POLICY_NAME);
	}
	
	public String getNewMessagingPoliciesDescription() {
		return getProperty(KEY_MESSAGING_POLICY_DESC);
	}
	
	public String getDisconnectedClientNotification() {
		return getProperty(KEY_DISCONNECTED_CLIENT_NOTIFICATION);
	}
	
	public String getNewUserID() {
		return getProperty(KEY_USER_ID);
	}
	
	public String getNewDestination() {
		return getProperty(KEY_DESTINATION);
	}
	
	public String getNewMaxMessages() {
		return getProperty(KEY_MAX_MESSAGES);
	}
	
	public String getNewClientAddress() {
		return getProperty(KEY_CLIENT_ADDRESS);
	}
	
	public String getNewCommonName() {
		return getProperty(KEY_COMMON_NAME);
	}
	
	public String getNewClientID() {
		return getProperty(KEY_CLIENT_ID);
	}
	
	public String getNewGroupID() {
		return getProperty(KEY_GROUP_ID);
	}
	
	public String getNewDestinationType() {
		return getProperty(KEY_DESTINATION_TYPE);
	}
	
	public String getNewMaxMessagesBehavior() {
		return getProperty(KEY_MAX_MESSAGES_BEHAVIOR_TYPE);
	}
	
	public String getMaxMessageTTL() {
		return getProperty(KEY_MAX_MESSAGE_TTL);
	}
	
	public boolean isDisconnectedClientNotification() {
		boolean result = false;
		String s = getDisconnectedClientNotification();
		if (s != null && s.equalsIgnoreCase("true")) {
			result = true;
		}
		return result;
	}
		
	public boolean isTopic() {
		boolean result = false;
		String s = getNewDestinationType();
		if (s != null && s.equalsIgnoreCase(DESTINATION_TYPE_TOPIC)) {
			result = true;
		}
		return result;
	}
	
	public boolean isQueue() {
		boolean result = false;
		String s = getNewDestinationType();
		if (s != null && s.equalsIgnoreCase(DESTINATION_TYPE_QUEUE)) {
			result = true;
		}
		return result;
	}
	
	public boolean isSharedSubscription() {
		boolean result = false;
		String s = getNewDestinationType();
		if (s != null && s.equalsIgnoreCase(DESTINATION_TYPE_SUBSCRIPTION)) {
			result = true;
		}
		return result;
	}
	
	public boolean isMaxMsgBehaviorReject() {
		boolean result = false;
		String s = getNewMaxMessagesBehavior();
		if (s != null && s.equalsIgnoreCase(MAX_MESSAGES_BEHAVIOR_TYPE_REJECT)) {
			result = true;
		}
		return result;
	}
	
	public boolean isMaxMsgBehaviorDiscard() {
		boolean result = false;
		String s = getNewMaxMessagesBehavior();
		if (s != null && s.equalsIgnoreCase(MAX_MESSAGES_BEHAVIOR_TYPE_DISCARD)) {
			result = true;
		}
		return result;
	}
}
