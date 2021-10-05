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
package com.ibm.ism.svt.testcase;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.svt.common.IsmSvtTestData;
import com.ibm.ism.svt.tasks.SubscriberTasks;

public class IsmSampleSub_Test1 {

		/**
		 * @param args Test arguments
		 */
		public static void main(String[] args) {
			IsmSampleSub_Test1 test = new IsmSampleSub_Test1();
			System.exit(test.runTest(new IsmSvtTestData(args)) ? 0 : 1);
		}
		
		public boolean runTest(IsmSvtTestData testData) {
			boolean result = false;
			String testDesc = "Subscribe 69 QoS=0 messages";
			
		  	 boolean wss = false;
			 boolean retain = false;
			 boolean cleanSession = true;
			 String uid = "admin";
			 String password = "admin";
			 String ip = "10.10.3.61";
			 String port = "16102";
			 String clientId = "fudgeit";
			 String msgTopic = "/test";
			 String msgCount = "69";
			 String msgText = "Two msgs on topic /test sent to fudgeit";
			 String subQoS = "0";

			 String browser = Platform.gsMozillaFirefox;
			 String url = "application/client_ship/web/ism_sample_sub.html";
			 
			VisualReporter.startLogging("Some Person", "ISM Sample Application - Subscriber", "ISM SVT");
			VisualReporter.logTestCaseStart(testDesc);
			
			try {
//				WebBrowser.start(testData.getBrowser());
//				WebBrowser.loadUrl(testData.getURL());
				WebBrowser.start(browser);
				WebBrowser.loadUrl(ip+":"+port+"/"+url);
				
				if (!SubscriberTasks.verifyPageLoaded() ) {
					throw new Exception("ISM Sample Application Subscriber page verification failed.");
				}
				if (!SubscriberTasks.subscribe(wss, uid, password, 
						ip, port, clientId, cleanSession, msgTopic, 
//	Pub only opts:					testData.getMsgCount(), testData.getMsgText(), testData.getRetain(), 
						subQoS ) ) {
					throw new Exception("Subscribe failed for some reason :-(");
				}
					
	/*			
				if (!LoginTasks.login(testData.getUser(), testData.getPassword())) {
					throw new Exception("Login failed for " + testData.getUser() + "/" + testData.getPassword());
				}
				if (!LicenseAgreementTasks.verifyPageLoaded() ) {
					throw new Exception("License agreement page verification failed.");
				}
				result = LicenseAgreementTasks.acceptLicense();
	*/			
			} catch (Exception e) {
				VisualReporter.failTest("Unexpected failure", e);
			}
			WebBrowser.shutdown();
			VisualReporter.stopLogging();
			if (!result) {
				VisualReporter.failTest(testDesc + " FAILED!");
			}
			return result;
		}


}
