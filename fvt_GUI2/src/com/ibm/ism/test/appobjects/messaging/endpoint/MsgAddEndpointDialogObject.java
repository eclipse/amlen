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

package com.ibm.ism.test.appobjects.messaging.endpoint;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class MsgAddEndpointDialogObject {
	public static final String TITLE_ADD_ENDPOINT = "Add Endpoint";
	public static final String DIALOG_DESC = "An endpoint is a port that client applications can connect to. An endpoint must have at least one connection policy and one messaging policy";
	public static final String ENDPOINT_DESCRIPTION = "Endpoint1";
	public static final String IPADDRESS = "All";
	public static final String SECURITY_PROFILE = "None";
	public static final String TITLE_ADD_CP_TO_ENDPOINT = "Add Connection Policies to the Endpoint";
	public static final String TITLE_ADD_MP_TO_ENDPOINT = "Add Messaging Policies to the Endpoint";
	public static final String TITLE_ADD_CP_TO_ENDPOINT_DESC = "A connection policy authorizes clients to connect to IBM MessageSight endpoints.";
	public static final String TITLE_ADD_MP_TO_ENDPOINT_DESC = "A messaging policy allows you to control what topics, queues, or global-shared subscriptions a client can access on IBM MessageSight.";
	public static final String CONNECTION_POLICY_NUMBER_ONE = "ONE";
	public static final String CONNECTION_POLICY_NUMBER_TWO = "TWO";
	public static final String CONNECTION_POLICY_NUMBER_THREE = "THREE";
	public static final String CONNECTION_POLICY_NUMBER_FOUR = "FOUR";
	public static final String CONNECTION_POLICY_NUMBER_SIXTEEN = "SIXTEEN";
	public static final String MESSAGING_POLICY_NUMBER_ONE = "ONE";
	public static final String MESSAGING_POLICY_NUMBER_TWO = "TWO";
	public static final String MESSAGING_POLICY_NUMBER_THREE = "THREE";
	public static final String MESSAGING_POLICY_NUMBER_FOUR = "FOUR";
	public static final String MESSAGING_POLICY_NUMBER_SIXTEEN = "SIXTEEN";
	
	public static SeleniumTestObject getTab_Endpoint() {
		return new SeleniumTestObject("//span[@id='ismMHTabContainer_tablist_ismMHEndpoints']");
	}

	public static SeleniumTextTestObject getTextEntry_EndpointID() {
		return new SeleniumTextTestObject("//input[@id='addismEndpointListDialog_name']");
	}

	public static SeleniumTextTestObject getTextEntry_EndpointDesc() {
		return new SeleniumTextTestObject("//textarea[@id='addismEndpointListDialog_description']");
	}

	public static SeleniumTextTestObject getCheckbox_EndpointEnabled() {
		return new SeleniumTextTestObject("//input[@id='addismEndpointListDialog_enabled']");
	}

	public static SeleniumTextTestObject getLink_AddProtocols() {
		return new SeleniumTextTestObject("//*[@id='addismEndpointListDialog_editProtocol']");
	}
	
	public static SeleniumTextTestObject getTextEntry_EndpointPort() {
		return new SeleniumTextTestObject("//input[@id='addismEndpointListDialog_port']");
	}
	
	public static SeleniumTextTestObject getTextEntry_EndpointMaxMessageSize() {
		return new SeleniumTextTestObject("//input[@id='addismEndpointListDialog_maxMessageSize']");
	}
	
	public static SeleniumTestObject getDownArrow_DestinationType() {
		return new SeleniumTestObject("//td[@id='addismMessagingPolicyListDialog_destinationType_downArrow']");
	}
	
	public static SeleniumTestObject getDownArrow_IPAddress() {
		return new SeleniumTestObject("//table[@id='addismEndpointListDialog_ipAddr']/tbody/tr/td[2]/div");
	}
	
	public static SeleniumTestObject getDownArrow_SecurityProfile() {
		return new SeleniumTestObject("//table[@id='addismEndpointListDialog_securityProfile']/tbody/tr/td[2]/div");
	}
	
	public static SeleniumTestObject getMenuItem1_IPAddress() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_12_text']");
	}
	
	public static SeleniumTestObject getMenuItem2_IPAddress() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_13_text']");
	}
	
	public static SeleniumTestObject getMenuItem1_SecurityProfile() {
		//return new SeleniumTestObject("//td[@id='dijit_MenuItem_14_text']");
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_12_text']");
	}
	
	public static SeleniumTestObject getMenuItem2_SecurityProfile() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_15_text']");
	}
	
	public static SeleniumTestObject getMenuItem3_SecurityProfile() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_16_text']");
	}
	

	
	public static SeleniumTestObject getGridIcon_EndPointAdd() {
		return new SeleniumTestObject("//span[@id='ismEndpointListGrid_add']");
  }
	
	public static SeleniumTestObject getGridIcon_ConnectionPoliciesAdd() {
		return new SeleniumTestObject("//span[@id='addismEndpointListDialog_connectionPoliciesReferencedObjectGrid_add']");
  }
	
	public static SeleniumTestObject getGridIcon_MessagingPoliciesAdd() {
		return new SeleniumTestObject("//span[@id='addismEndpointListDialog_messagingPoliciesReferencedObjectGrid_add']");
  }
	
	//public static SeleniumTestObject getButton_Add() {
	//	return new SeleniumTestObject("//span[@id='dijitReset.dijitInline.dijitIcon.ismiconAdd']");
	//}
	
	public static SeleniumTestObject getMenuItem1_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_0']");
	}
	
	public static SeleniumTestObject getMenuItem2_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_1']");
	}
	public static SeleniumTestObject getMenuItem3_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_2']");
	}
	
	public static SeleniumTestObject getMenuItem4_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_3']");
	}
	
	public static SeleniumTestObject getMenuItem5_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_4']");
		
	}	
	
	public static SeleniumTestObject getMenuItem6_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_5']");
		
	}	
	
	public static SeleniumTestObject getMenuItem7_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_6']");
		
	}	
	
	public static SeleniumTestObject getMenuItem8_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_7']");
		
	}	
	
	public static SeleniumTestObject getMenuItem9_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_8']");
		
	}	
	
	public static SeleniumTestObject getMenuItem10_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_9']");
		
	}	
	
	public static SeleniumTestObject getMenuItem11_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_10']");
		
	}

	public static SeleniumTestObject getMenuItem12_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_11']");
		
	}	
	
	public static SeleniumTestObject getMenuItem13_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_12']");
		
	}	
	
	public static SeleniumTestObject getMenuItem14_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_13']");
		
	}	
	
	public static SeleniumTestObject getMenuItem15_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_14']");
		
	}	
	
	public static SeleniumTestObject getMenuItem16_EndpointConnectionPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_0']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_4']");
		
	}	
			
		
	
	public static SeleniumTestObject getMenuItem1_EndpointMessagingPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_10']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_10']");
	}
	
	public static SeleniumTestObject getMenuItem2_EndpointMessagingPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_10']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_11']");
	}
	
	public static SeleniumTestObject getMenuItem3_EndpointMessagingPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_10']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_12']");
	}
	
	public static SeleniumTestObject getMenuItem4_EndpointMessagingPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_10']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_13']");
	}
	
	public static SeleniumTestObject getMenuItem16_EndpointMessagingPolicy() {
		//return new SeleniumTestObject("//div[@id='widget_idx_form_CheckBox_10']");
		return new SeleniumTestObject("//input[@id='idx_form_CheckBox_14']");
	}
	
	
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addismEndpointListDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_24']");
	}

	public static SeleniumTestObject getButton_CP_Save() {
		return new SeleniumTestObject("//span[@id='addConnectionPoliciesismEndpointListDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_CP_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_15']");
	}

	public static SeleniumTestObject getButton_MP_Save() {
		return new SeleniumTestObject("//span[@id='addMessagingPoliciesismEndpointListDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_MP_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_18']");
	}



}



