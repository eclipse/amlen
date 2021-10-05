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
import com.ibm.ism.svt.tasks.PublisherTasks;

public class IsmSamplePub_Test1 {
	static String url = "http://10.10.10.10/application/client_ship/web/ism_sample_pub.html";
	static String user = "admin";
	static String pwd = "admin";
	static String browser = Platform.gsMozillaFirefox;

	
	/**
	 * @param args Test arguments
	 */
	public static void main(String[] args) {

		for (String arg : args) {
			String strs[] = arg.split("=");
			if (strs[0].equalsIgnoreCase("URL")) {
				url = strs[1];
			} else if (strs[0].equalsIgnoreCase("USER")) {
				user = strs[1];
			} else if (strs[0].equalsIgnoreCase("PASSWORD")) {
				pwd = strs[1];
			} else if (strs[0].equalsIgnoreCase("BROWSER")) {
				browser = strs[1];
			}
		}

		
//jcd		IsmSamplePub_Test1 test = new IsmSamplePub_Test1();
//		System.exit(test.runTest(new IsmSvtTestData(args)) ? 0 : 1);
//	}
//	
//	public boolean runTest(IsmSvtTestData testData) {
		boolean result = false;
		String testDesc = "Publish 69 QoS=0 messages";
		
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
		 String msgText = "Two msgs on topic /test sent by fudgeit";
		 String pubQoS = "0";

//		 String browser = Platform.gsMozillaFirefox;
//		 String url = "application/client_ship/web/ism_sample_pub.html";
		
		VisualReporter.startLogging("Some Person", "ISM Sample Application - Publisher", "ISM SVT");
		VisualReporter.logTestCaseStart(testDesc);
		
		try {
//			WebBrowser.start(testData.getBrowser());
//			WebBrowser.loadUrl(testData.getURL());
			WebBrowser.start(browser);
//jcd			WebBrowser.loadUrl(ip+":"+port+"/"+url);
			WebBrowser.loadUrl(url);
			
			if (!PublisherTasks.verifyPageLoaded() ) {
				throw new Exception("ISM Sample Application Publisher page verification failed.");
			}
			if (!PublisherTasks.publish( wss, uid, password, 
					ip, port, clientId, cleanSession,  
					msgTopic, msgCount, msgText, pubQoS, retain ) ) {
				throw new Exception("Publish failed for some reason :-(");
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
			VisualReporter.failTest(testDesc + " failed.");
		}
//jcd		return result;
		return;
	}

}
