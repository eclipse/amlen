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

package com.ibm.ism.test.tasks.firststeps;

import com.ibm.automation.core.web.WebBrowser;

public class MessagingTab {
	
		
		private static final String MESSAGE_HUBS_HEADER = "Message Hubs";
		private static final String MESSAGE_QUEUES_HEADER = "Message Queues";
		private static final String MQ_CONNECTIVITY_HEADER = "MQ Connectivity";
		private static final String USERS_AND_GROUPS_HEADER = "Users and Groups";
		
		
		public static boolean verifyPageLoaded() {
			WebBrowser.waitForReady();
			
			if (WebBrowser.waitForText(MESSAGE_HUBS_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MESSAGE_QUEUES_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MQ_CONNECTIVITY_HEADER, 30) == false) {
				return false;
			}
			
			if (WebBrowser.waitForText(USERS_AND_GROUPS_HEADER, 30) == false) {	
				return false;
			}
			
			return true;
		}
		

}
