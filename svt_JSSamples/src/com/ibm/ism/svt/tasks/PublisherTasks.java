// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.svt.tasks;



import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.svt.application.PublisherObjects;

import com.ibm.ism.svt.common.IsmSvtTest;
//import com.ibm.ism.svt.common.IsmSvtTestData;
import com.ibm.ism.svt.common.IsmSvtConstants;

/*
 * Task to be driven off the ISM Sample Application web page:  ism_sample_pub.html 
 */
public class PublisherTasks implements IsmSvtConstants {

	/*
	 * verifyPageLoaded
	 */
	public static boolean verifyPageLoaded() {
		WebBrowser.waitForReady();
		return WebBrowser.isTextPresent("ISM WebSocket Sample");
	}
	
	/*
	 * publish()
	 */
	public static boolean publish(boolean wss, String uid, String password, String ip, String port, String clientId, boolean cleanSession, String msgTopic, String msgCount, String msgText, String pubQoS, boolean retain ){
		try {
		// Secure Socket Layer (SSL)
		PublisherObjects.getCheckBox_ismWSS().focus();
			IsmSvtTest.delay(STATIC_WAIT_TIME);
		if ( wss )
		{
			PublisherObjects.getCheckBox_ismWSS().check();		// Toggle the check-box			
		} else {
			PublisherObjects.getCheckBox_ismWSS().uncheck();		// Toggle the check-box			
		}

		// userid
		PublisherObjects.getTextEntry_ismUID().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_ismUID().setText(uid);

		// password
		PublisherObjects.getTextEntry_ismPassword().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_ismPassword().setPassword(password);

		// ISM Server IP address
		PublisherObjects.getTextEntry_ismIP().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_ismIP().setText(ip);

		// ISM Server IP port
		PublisherObjects.getTextEntry_ismPort().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_ismPort().setText(port);

		// MQTT Client Id
		PublisherObjects.getTextEntry_ismClientId().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_ismClientId().setText(clientId);

		// Clean Session (Check-box)
		PublisherObjects.getCheckBox_ismCleanSession().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		if ( cleanSession  ){
			PublisherObjects.getCheckBox_ismCleanSession().check();				
		} else {
			PublisherObjects.getCheckBox_ismCleanSession().uncheck();				
		}

		// msgTopic
		PublisherObjects.getTextEntry_msgTopic().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_msgTopic().setText(msgTopic);

		// msgCount
		PublisherObjects.getTextEntry_msgCount().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_msgCount().setText(msgCount);

		// msgText
		PublisherObjects.getTextEntry_msgText().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getTextEntry_msgText().setText(msgText);

		// pubQoS  (Select - WebListBox object)
		PublisherObjects.getSelect_pubQoS().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		PublisherObjects.getSelect_pubQoS().select(pubQoS);
		
		// Retain (Check-box)
		PublisherObjects.getCheckBox_retain().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		if (  retain ){
			PublisherObjects.getCheckBox_retain().check();				
		} else {
			PublisherObjects.getCheckBox_retain().uncheck();				
		}
		
		// Push the Publish Button
		PublisherObjects.getButton_pubConnect().focus();
		IsmSvtTest.delay(SVT_DELAY);
		PublisherObjects.getButton_pubConnect().click();
		
		IsmSvtTest.waitForBrowserReady( 100 );
		return validatePublish( msgCount, msgTopic );
		
	} catch (Exception e) {
		//VisualReporter.logScreenCapture("Login failed", true);
		VisualReporter.failTest("PUBLISH FAILED During Page Setup!", e);
		return false;
	}
	}
	
	/*
	 * validatePublish()
	 */
	public static boolean validatePublish (String msgCount, String msgTopic) {

		/*
		Publishing 3 messages on topic: topicA.
		Data Store: false.
		Publish complete. Disconnected from ISM Server.
		*/
		if ( WebBrowser.isTextPresent("Publishing "+msgCount+" messages on topic: "+msgTopic+".") ){
		// WebBrowser.isTextPresent("Data Store: false.");
		return WebBrowser.isTextPresent("Publish complete. Disconnected from ISM Server.");
		}
		VisualReporter.failTest("PUBLISH FAILED Publish Validation!");
		return false;
	}

	/*
	 * validateLostConnection()
	 */
	public static boolean validateLostConnection () {

		/*		
		Subscriber connection lost for client jcd.
		 */
		PublisherObjects.getTextArea_messages().focus();
		PublisherObjects.getTextArea_messages().click();
		IsmSvtTest.delay(SVT_DELAY);
		String theText = PublisherObjects.getTextArea_messages().getText();
		
		//Does it match expected Text?
		if ( theText.equals( SVT_LOSTCONNECTION ) ) { 	
			VisualReporter.failTest("PUBLISH FAILED Lost Connection!");
			return true;
		} else {
			return false;
		}
	}

	/*
	 * clearMessagesTextArea()
	 */
	public static boolean clearMessagesTextArea () {

		PublisherObjects.getTextArea_messages().focus();
		IsmSvtTest.delay(SVT_DELAY);
		PublisherObjects.getTextArea_messages().clearText();
		
		return true;
	}

}
