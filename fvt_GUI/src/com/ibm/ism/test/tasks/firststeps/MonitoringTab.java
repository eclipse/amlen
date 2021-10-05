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

public class MonitoringTab {

	
		
		private static final String MONITORING_CONNECTIONS_HEADER = "Connection Monitor";
		private static final String MONITORING_ENDPOINTS_HEADER = "Endpoint Monitor";
		private static final String MONITORING_QUEUES_HEADER = "Queue Monitor";
		private static final String MONITORING_TOPICS_HEADER = "Topic Monitor";
		private static final String MONITORING_MQTT_CLIENTS_HEADER = "MQTT Clients";
		private static final String MONITORING_SUBSCRIPTIONS_HEADER = "Subscription Monitor";
		private static final String MONITORING_APPLIANCE_HEADER = "Appliance Monitor";
		private static final String MONITORING_DOWNLOAD_LOGS_HEADER = "Logs";
		
		
		public static boolean verifyPageLoaded() {
			WebBrowser.waitForReady();
			
			if (WebBrowser.waitForText(MONITORING_CONNECTIONS_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MONITORING_ENDPOINTS_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MONITORING_QUEUES_HEADER, 30) == false) {
				return false;
			}
			
			if (WebBrowser.waitForText(MONITORING_TOPICS_HEADER, 30) == false) {	
				return false;
			}
			if (WebBrowser.waitForText(MONITORING_MQTT_CLIENTS_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MONITORING_SUBSCRIPTIONS_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MONITORING_APPLIANCE_HEADER, 30) == false) {
				return false;
			}
			if (WebBrowser.waitForText(MONITORING_DOWNLOAD_LOGS_HEADER, 30) == false) {
				return false;
			}
			
			return true;
		}
		

}
