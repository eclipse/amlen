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

package com.ibm.ism.test.appobjects.messaging.messagingpolicy;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class MsgAddMessagingPolicyDialogObject {
	public static final String TITLE_ADD_MESSAGING_POLICY = "Add Messaging Policy";
	public static final String DIALOG_DESC = "A messaging policy authorizes connected clients to perform specific messaging actions, such as which topics, queues or shared durable subscriptions the client can access on IBM MessageSight.";
	
	
	public static final String ID_TEXT_BOX_MAX_MESSAGE_TIME_TO_LIVE = "addismMessagingPolicyListDialog_maxMessageTimeToLive";
	public static final String ID_CHECK_BOX_DISCONNECTED_CLIENT_NOTIFICATION = "addismMessagingPolicyListDialog_DisconnectedClientNotification";
	
	public static SeleniumTestObject getTab_MessagingPolicy() {
		return new SeleniumTestObject("//span[@id='ismMHTabContainer_tablist_ismMHMessagingPolicies']");
	}

	public static SeleniumTextTestObject getTextEntry_MessagingPolicyID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_name']");
	}

	public static SeleniumTextTestObject getTextEntry_MessagingPolicyDesc() {
		//return new SeleniumTextTestObject("//input[@id='ism_widgets_TextArea_0']");
		return new SeleniumTextTestObject("//textarea[@id='addismMessagingPolicyListDialog_description']");
	}

	public static SeleniumTextTestObject getTextEntry_DestinationTypeID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_destinationType_comboBox']");
	}
	
	public static SeleniumTestObject getDownArrow_DestinationType() {
		//return new SeleniumTestObject("//table[@id='ism_widgets_CheckBoxSelect_1']/tbody/tr/td[2]/div");
		return new SeleniumTestObject("//td[@id='addismMessagingPolicyListDialog_destinationType_downArrow']");
	}
	
	public static SeleniumTestObject getDownArrow_MaxMessagesBehavior() {
		//return new SeleniumTestObject("//table[@id='ism_widgets_CheckBoxSelect_1']/tbody/tr/td[2]/div");
		return new SeleniumTestObject("//td[@id='addismMessagingPolicyListDialog_maxMessagesBehavior_downArrow']");
	}

	//public static SeleniumTestObject getCheckBox_Topic() throws Exception {
	public static SeleniumTestObject getCheckBox_Topic() {
		//return new SeleniumTestObject("//td[@id='dijit_MenuItem_16_text']");
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_12_text']");
	}
	
	public static SeleniumTestObject getMenuItem_DestimationTypeTopic() {
		return new SeleniumTestObject("//table[@id='addismMessagingPolicyListDialog_destinationType_menu']/tbody/tr/td");
	}

	public static SeleniumTestObject getCheckBox_Queue() {
		//return new SeleniumTestObject("//td[@id='dijit_MenuItem_18_text']");
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_14_text']");
	}

	public static SeleniumTestObject getMenuItem_DestimationTypeQueue() {
		return new SeleniumTestObject("//table[@id='addismMessagingPolicyListDialog_destinationType_menu']/tbody/tr[2]/td");
	}
	
	public static SeleniumTestObject getCheckBox_Subscription() {
		//return new SeleniumTestObject("//td[@id='dijit_MenuItem_17_text']");
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_13_text']");
		
	}
	
	public static SeleniumTestObject getMenuItem_DestimationTypeSubscription() {
		return new SeleniumTestObject("//table[@id='addismMessagingPolicyListDialog_destinationType_menu']/tbody/tr[1]/td");
	}
	
	public static SeleniumTextTestObject getTextEntry_Destination() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_destination']");
	}
	
	public static SeleniumTextTestObject getTextEntry_MaxMessages() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_maxMessages']");
	}
	
	public static SeleniumTestObject getMenuItem_RejectNewMessages() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_15_text']");
	}
	
	public static SeleniumTestObject getMenuItem_DiscardOldestMessages() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_16_text']");
	}
	
	public static SeleniumTestObject getCheckBox_DisconnectedClientNotification() {
		return new SeleniumTextTestObject("//input[@id='" + ID_CHECK_BOX_DISCONNECTED_CLIENT_NOTIFICATION + "']");
	}
	
	public static SeleniumTestObject getCheckBox_Publish() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem0']");
	}
	
	public static SeleniumTestObject getCheckBox_Subscribe() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem1']");
	}
	
	public static SeleniumTestObject getCheckBox_Send() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem0']");
	}
	
	public static SeleniumTestObject getCheckBox_Browse() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem1']");
	}
	
	public static SeleniumTestObject getCheckBox_Queue_Receive() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem2']");
	}

	public static SeleniumTestObject getCheckBox_Receive() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem0']");
	}
	
	public static SeleniumTestObject getCheckBox_Control() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_action_CheckItem1']");
	}
	
	
	public static SeleniumTestObject getCheckBox_clientAddress() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_checkbox_clientAddress']");
	}


	public static SeleniumTextTestObject getTextEntry_ClientIPAddress() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_clientAddress']");
	}
	
	
	public static SeleniumTestObject getCheckBox_UserID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_checkbox_UserID']");
	}
	
	public static SeleniumTextTestObject getTextEntry_UserID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_UserID']");
	}
	
	
	public static SeleniumTestObject getCheckBox_CertificateCommonName() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_checkbox_commonNames']");
	}
	
	public static SeleniumTextTestObject getTextEntry_CertificateCommonName() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_commonNames']");
	}
	
	
	public static SeleniumTestObject getCheckBox_ClientID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_checkbox_clientID']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ClientID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_clientID']");
	}
	
	
	public static SeleniumTestObject getCheckBox_GroupID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_checkbox_GroupID']");
	}
	
	public static SeleniumTextTestObject getTextEntry_GroupID() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_GroupID']");
	}
	
	public static SeleniumTestObject getCheckBox_Protocol() {
		return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_checkbox_protocol']");
	}
	
	public static SeleniumTestObject getDownArrow_SelectProtocol() {
		return new SeleniumTestObject("//table[@id='addismMessagingPolicyListDialog_protocol']/tbody/tr/td[2]/div");
	}
	
	
	public static SeleniumTestObject getCheckBox_JMS() {
		//return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_protocol_CheckItem0']");
		return new SeleniumTextTestObject("//div[@id='addismMessagingPolicyListDialog_protocol_menu']/table/tbody/tr/td/table/tbody/tr");
	}
	
	public static SeleniumTestObject getCheckBox_MQTT() {
		//return new SeleniumTextTestObject("//input[@id='addismMessagingPolicyListDialog_protocol_CheckItem1']");
		return new SeleniumTextTestObject("//div[@id='addismMessagingPolicyListDialog_protocol_menu']/table/tbody/tr/td/table/tbody/tr[2]");
		
	}
	
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addismMessagingPolicyListDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_10']");
	}

	public static SeleniumTextTestObject getTextEntry_MaxMessageTTL() {
		return new SeleniumTextTestObject("//input[@id='" + ID_TEXT_BOX_MAX_MESSAGE_TIME_TO_LIVE + "']");
	}

}
